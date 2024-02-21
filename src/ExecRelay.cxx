// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "ExecRelay.hxx"
#include "Handler.hxx"
#include "Action.hxx"
#include "system/Error.hxx"
#include "io/Pipe.hxx"
#include "io/UniqueFileDescriptor.hxx"

#include <signal.h>
#include <unistd.h>

using std::string_view_literals::operator""sv;

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

	auto [stdin_r, stdin_w] = CreatePipeNonBlock();
	auto [stdout_r, stdout_w] = CreatePipeNonBlock();

	pid_t pid = fork();
	if (pid < 0)
		throw MakeErrno("fork() failed");

	if (pid == 0) {
		UnblockSignals();

		stdin_r.CheckDuplicate(FileDescriptor{STDIN_FILENO});
		stdout_w.CheckDuplicate(FileDescriptor{STDOUT_FILENO});

		/* disable O_NONBLOCK */
		FileDescriptor{STDIN_FILENO}.SetBlocking();
		FileDescriptor{STDOUT_FILENO}.SetBlocking();

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

	client.Request(stdin_w.Release(), stdout_r.Release(),
		       std::move(request));
	return true;
} catch (...) {
	handler.OnRelayError("Zinternal server error"sv,
			     std::current_exception());
	return false;
}
