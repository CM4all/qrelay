// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "RawExecRelay.hxx"
#include "ExitStatus.hxx"
#include "Handler.hxx"
#include "Action.hxx"
#include "djb/QmqpMail.hxx"
#include "lib/fmt/RuntimeError.hxx"
#include "system/Error.hxx"
#include "system/PidFD.h"
#include "io/Pipe.hxx"
#include "io/UniqueFileDescriptor.hxx"
#include "util/SpanCast.hxx"

#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

using std::string_view_literals::operator""sv;

RawExecRelay::RawExecRelay(EventLoop &event_loop, const QmqpMail &mail,
			   std::list<std::span<const std::byte>> &&additional_headers,
			   RelayHandler &_handler)
	:handler(_handler),
	 request_pipe(event_loop, BIND_THIS_METHOD(OnRequestPipeReady)),
	 response_pipe(event_loop, BIND_THIS_METHOD(OnResponsePipeReady))
{
	for (const auto &i : additional_headers)
		request_buffer.Push(i);
	request_buffer.Push(AsBytes(mail.message));
}

RawExecRelay::~RawExecRelay() noexcept
{
	request_pipe.Close();
	response_pipe.Close();

	if (pidfd)
		// TODO send SIGKILL after timeout?
		pidfd->Kill(SIGTERM);
}

static void
UnblockSignals() noexcept
{
	/* don't ignore SIGPIPE */
	for (auto i : {SIGPIPE})
		signal(i, SIG_DFL);

	/* unblock signals that were blocked in the daemon for
	   signalfd */
	sigset_t mask;
	sigfillset(&mask);
	sigprocmask(SIG_UNBLOCK, &mask, nullptr);
}

bool
RawExecRelay::Start(const Action &action) noexcept
try {
	assert(action.type == Action::Type::EXEC_RAW);
	assert(!action.exec.empty());

	auto [stdin_r, stdin_w] = CreatePipeNonBlock();
	auto [stdout_r, stdout_w] = CreatePipeNonBlock();

	pid_t pid = fork();
	if (pid < 0)
		throw MakeErrno("fork() failed");

	if (pid == 0) {
		UnblockSignals();

		/* disable O_NONBLOCK */
		stdin_r.SetBlocking();
		stdout_w.SetBlocking();

		stdin_r.CheckDuplicate(FileDescriptor{STDIN_FILENO});
		stdout_w.CheckDuplicate(FileDescriptor{STDOUT_FILENO});
		stdout_w.CheckDuplicate(FileDescriptor{STDERR_FILENO});

		char *argv[Action::MAX_EXEC + 1];

		unsigned n = 0;
		for (const auto &i : action.exec)
			argv[n++] = const_cast<char *>(i.c_str());

		argv[n] = nullptr;

		execv(argv[0], argv);
		_exit(1);
	}

	stdin_r.Close();
	stdout_w.Close();

	ExitListener &exit_listener = *this;
	pidfd.emplace(GetEventLoop(), UniqueFileDescriptor{my_pidfd_open(pid, 0)},
		      "exec_raw", exit_listener);

	response_pipe.Open(stdout_r.Release());
	response_pipe.ScheduleRead();

	request_pipe.Open(stdin_w.Release());
	return TryWrite();
} catch (...) {
	handler.OnRelayError("Zinternal server error"sv,
			     std::current_exception());
	return false;
}

bool
RawExecRelay::TryWrite() noexcept
try {
	switch (request_buffer.Write(request_pipe.GetFileDescriptor())) {
	case WriteBuffer::Result::MORE:
		request_pipe.ScheduleWrite();
		break;

	case WriteBuffer::Result::FINISHED:
		request_pipe.Close();
		break;
	}

	return true;
} catch (...) {
	handler.OnRelayError("Zwrite error"sv,
			     std::current_exception());
	return true;
}

void
RawExecRelay::OnRequestPipeReady(unsigned) noexcept
{
	TryWrite();
}

static std::size_t
ReadPipeOrThrow(FileDescriptor fd, std::span<std::byte> dest)
{
	const auto nbytes = fd.Read(dest);
	if (nbytes < 0)
		throw MakeErrno("Failed to read from pipe");

	return static_cast<std::size_t>(nbytes);
}

inline bool
RawExecRelay::TryRead() noexcept
try {
	const FileDescriptor fd = response_pipe.GetFileDescriptor();

	auto dest = std::span{response_buffer}.subspan(response_fill);

	if (dest.empty()) {
		/* buffer is full; discard the rest */
		std::array<std::byte, 1024> discard_buffer;
		return ReadPipeOrThrow(fd, discard_buffer) > 0;
	}

	const auto nbytes = ReadPipeOrThrow(fd, dest);
	response_fill += nbytes;
	return nbytes > 0;
} catch (...) {
	handler.OnRelayError("Zread error"sv,
			     std::current_exception());
	return true;
}

void
RawExecRelay::OnResponsePipeReady(unsigned) noexcept
{
	if (!TryRead()) {
		response_pipe.Close();

		if (!pidfd) {
			Finish();
			return;
		}
	}
}

void
RawExecRelay::OnChildProcessExit(int _status) noexcept
{
	pidfd.reset();

	status = _status;

	if (!response_pipe.IsDefined())
		Finish();
}

void
RawExecRelay::Finish() noexcept
{
	assert(!response_pipe.IsDefined());
	assert(!pidfd);

	std::string_view response = "K"sv;
	if (WIFSIGNALED(status))
		response = "Zinternal server error";
	else if (WEXITSTATUS(status) != EXIT_SUCCESS)
		response = ExitStatusToQmqpResponse(WEXITSTATUS(status));

	if (response_fill > 1) {
		response_buffer[0] = static_cast<std::byte>(response.front());
		std::span<const std::byte> response_span = std::span{response_buffer}.first(response_fill);
		response = ToStringView(response_span);
	}

	try {
		if (WIFSIGNALED(status))
			throw FmtRuntimeError("Process died from signal {}{}"sv,
					      WTERMSIG(status),
					      WCOREDUMP(status) ? " (core dumped)"sv : ""sv);
		else if (WEXITSTATUS(status) != EXIT_SUCCESS)
			throw FmtRuntimeError("Exit status {}",
					      WEXITSTATUS(status));
	} catch (...) {
		handler.OnRelayError(response, std::current_exception());
		return;
	}

	handler.OnRelayResponse(response);
}
