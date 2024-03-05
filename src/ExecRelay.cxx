// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "ExecRelay.hxx"
#include "ExitStatus.hxx"
#include "Handler.hxx"
#include "Action.hxx"
#include "lib/fmt/RuntimeError.hxx"
#include "system/Error.hxx"
#include "system/PidFD.h"
#include "io/Pipe.hxx"
#include "io/UniqueFileDescriptor.hxx"

#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

using std::string_view_literals::operator""sv;

ExecRelay::~ExecRelay() noexcept
{
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
ExecRelay::Start(const Action &action) noexcept
try {
	assert(action.type == Action::Type::EXEC);
	assert(!action.exec.empty());

	auto [stdin_r, stdin_w] = CreatePipe();
	auto [stdout_r, stdout_w] = CreatePipe();

	pid_t pid = fork();
	if (pid < 0)
		throw MakeErrno("fork() failed");

	if (pid == 0) {
		UnblockSignals();

		stdin_r.CheckDuplicate(FileDescriptor{STDIN_FILENO});
		stdout_w.CheckDuplicate(FileDescriptor{STDOUT_FILENO});

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
		      "exec", exit_listener);

	stdin_w.SetNonBlocking();
	stdout_r.SetNonBlocking();

	client.Request(stdin_w.Release(), stdout_r.Release(),
		       std::move(request));
	return true;
} catch (...) {
	handler.OnRelayError("Zinternal server error"sv,
			     std::current_exception());
	return false;
}

void
ExecRelay::OnChildProcessExit(int status) noexcept
{
	pidfd.reset();

	try {
		if (WIFSIGNALED(status))
			throw FmtRuntimeError("Process died from signal {}{}"sv,
					      WTERMSIG(status),
					      WCOREDUMP(status) ? " (core dumped)"sv : ""sv);
		else if (WEXITSTATUS(status) != EXIT_SUCCESS)
			throw FmtRuntimeError("Exit status {}",
					      WEXITSTATUS(status));
	} catch (...) {
		handler.OnRelayError(ExitStatusToQmqpResponse(WEXITSTATUS(status)),
				     std::current_exception());
		return;
	}

	if (deferred_response.data() != nullptr)
		BasicRelay::OnNetstringResponse(std::move(deferred_response));
}

void
ExecRelay::OnNetstringResponse(AllocatedArray<std::byte> &&payload) noexcept
{
	if (pidfd)
		deferred_response = std::move(payload);
	else
		BasicRelay::OnNetstringResponse(std::move(payload));
}
