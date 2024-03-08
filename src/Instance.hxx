// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include "Listener.hxx"
#include "lua/ReloadRunner.hxx"
#include "lua/State.hxx"
#include "lua/ValuePtr.hxx"
#include "spawn/ZombieReaper.hxx"
#include "net/UniqueSocketDescriptor.hxx"
#include "io/Logger.hxx"
#include "event/Loop.hxx"
#include "event/ShutdownListener.hxx"
#include "event/SignalEvent.hxx"
#include "event/systemd/Watchdog.hxx"
#include "config.h"

#include <forward_list>

namespace Lua { class Value; }

class Instance {
	EventLoop event_loop;
	ShutdownListener shutdown_listener{event_loop, BIND_THIS_METHOD(OnShutdown)};
	SignalEvent sighup_event;

	ZombieReaper zombie_reaper{event_loop};

#ifdef HAVE_LIBSYSTEMD
	Systemd::Watchdog systemd_watchdog{event_loop};
#endif

	Lua::State lua_state;

	Lua::ReloadRunner reload{lua_state.get()};

	UniqueSocketDescriptor log_socket;

	std::forward_list<QmqpRelayListener> listeners;

public:
	RootLogger logger;

	Instance();

	EventLoop &GetEventLoop() {
		return event_loop;
	}

	lua_State *GetLuaState() {
		return lua_state.get();
	}

	SocketDescriptor GetLogSocket() const noexcept {
		return log_socket;
	}

	void AddListener(UniqueSocketDescriptor &&fd,
			 size_t max_size,
			 Lua::ValuePtr &&handler) noexcept;

	void AddListener(SocketAddress address,
			 size_t max_size,
			 Lua::ValuePtr &&handler);

#ifdef HAVE_LIBSYSTEMD
	/**
	 * Listen for incoming connections on sockets passed by systemd
	 * (systemd socket activation).
	 */
	void AddSystemdListener(size_t max_size, Lua::ValuePtr &&handler);
#endif // HAVE_LIBSYSTEMD

	void Check();
	void SetupLogSocket();

private:
	void OnShutdown() noexcept;
	void OnReload(int) noexcept;
};
