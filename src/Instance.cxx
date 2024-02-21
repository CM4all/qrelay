// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "Instance.hxx"
#include "net/SocketConfig.hxx"

extern "C" {
#include <lauxlib.h>
}

#ifdef HAVE_LIBSYSTEMD
#include <systemd/sd-daemon.h>
#endif

#include <stdexcept>

#include <errno.h>
#include <string.h>

Instance::Instance()
	:sighup_event(event_loop, SIGHUP, BIND_THIS_METHOD(OnReload)),
	 lua_state(luaL_newstate())
{
	shutdown_listener.Enable();
	sighup_event.Enable();
}

inline void
Instance::AddListener(UniqueSocketDescriptor &&fd,
		      size_t max_size,
		      Lua::ValuePtr &&handler) noexcept
{
	listeners.emplace_front(event_loop, max_size, std::move(handler),
				logger, event_loop);
	listeners.front().Listen(std::move(fd));
}

static UniqueSocketDescriptor
MakeListener(SocketAddress address)
{
	constexpr int socktype = SOCK_STREAM;

	SocketConfig config(address);
	config.mode = 0666;

	/* we want to receive the client's UID */
	config.pass_cred = true;

	config.listen = 64;

	return config.Create(socktype);
}

void
Instance::AddListener(SocketAddress address,
		      size_t max_size,
		      Lua::ValuePtr &&handler)
{
	AddListener(MakeListener(address), max_size, std::move(handler));
}

#ifdef HAVE_LIBSYSTEMD

void
Instance::AddSystemdListener(size_t max_size, Lua::ValuePtr &&handler)
{
	int n = sd_listen_fds(true);
	if (n < 0) {
		logger(1, "sd_listen_fds() failed: ", strerror(errno));
		return;
	}

	if (n == 0) {
		logger(1, "No systemd socket");
		return;
	}

	for (unsigned i = 0; i < unsigned(n); ++i)
		AddListener(UniqueSocketDescriptor(SD_LISTEN_FDS_START + i),
			    max_size,
			    Lua::ValuePtr(handler));
}

#endif // HAVE_LIBSYSTEMD

void
Instance::Check()
{
	if (listeners.empty())
		throw std::runtime_error("No QMQP listeners configured");
}

void
Instance::OnShutdown() noexcept
{
	shutdown_listener.Disable();
	sighup_event.Disable();

#ifdef HAVE_LIBSYSTEMD
	systemd_watchdog.Disable();
#endif

	event_loop.Break();
}

void
Instance::OnReload(int) noexcept
{
	reload.Start();
}
