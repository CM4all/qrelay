// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "Instance.hxx"
#include "lua/net/SocketAddress.hxx"
#include "net/ConnectSocket.hxx"
#include "net/SocketConfig.hxx"
#include "net/AllocatedSocketAddress.hxx"
#include "net/log/Protocol.hxx"
#include "util/ScopeExit.hxx"

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
	listeners.emplace_front(event_loop, *this, max_size, std::move(handler),
				logger);
	listeners.front().Listen(std::move(fd));
}

static UniqueSocketDescriptor
MakeListener(SocketAddress address)
{
	constexpr int socktype = SOCK_STREAM;

	const SocketConfig config{
		.bind_address = AllocatedSocketAddress{address},
		.listen = 64,
		.mode = 0666,

		/* we want to receive the client's UID */
		.pass_cred = true,
	};

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
Instance::SetupLogSocket()
{
	const auto L = lua_state.get();

	lua_getglobal(L, "log_server");
	AtScopeExit(L) { lua_pop(L, 1); };
	if (lua_isnil(L, -1))
		return;

	const auto address = Lua::ToSocketAddress(L, -1, Net::Log::DEFAULT_PORT);
	log_socket = CreateConnectDatagramSocket(address);
}

void
Instance::OnShutdown() noexcept
{
	shutdown_listener.Disable();
	sighup_event.Disable();
	zombie_reaper.Disable();

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
