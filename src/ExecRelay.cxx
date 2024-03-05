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
#include "util/ScopeExit.hxx"

#include <signal.h>
#include <spawn.h>
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

bool
ExecRelay::Start(const Action &action) noexcept
try {
	assert(action.type == Action::Type::EXEC);
	assert(!action.exec.empty());

	auto [stdin_r, stdin_w] = CreatePipe();
	auto [stdout_r, stdout_w] = CreatePipe();

	posix_spawnattr_t attr;
	posix_spawnattr_init(&attr);
	AtScopeExit(&attr) { posix_spawnattr_destroy(&attr); };

	sigset_t signals;
	sigemptyset(&signals);
	posix_spawnattr_setsigmask(&attr, &signals);
	sigaddset(&signals, SIGHUP);
	posix_spawnattr_setsigdefault(&attr, &signals);

	posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSIGDEF|POSIX_SPAWN_SETSIGMASK);

	posix_spawn_file_actions_t file_actions;
	posix_spawn_file_actions_init(&file_actions);
	AtScopeExit(&file_actions) { posix_spawn_file_actions_destroy(&file_actions); };

        posix_spawn_file_actions_adddup2(&file_actions, stdin_r.Get(), STDIN_FILENO);
        posix_spawn_file_actions_adddup2(&file_actions, stdout_w.Get(), STDOUT_FILENO);

	char *argv[Action::MAX_EXEC + 1];

	unsigned n = 0;
	for (const auto &i : action.exec)
		argv[n++] = const_cast<char *>(i.c_str());

	argv[n] = nullptr;

	static constexpr const char *env[]{nullptr};

	pid_t pid;
	if (posix_spawn(&pid, argv[0], &file_actions, &attr,
			const_cast<char *const *>(argv),
			const_cast<char *const *>(env)) != 0)
		throw MakeErrno("Failed to execute process");

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
