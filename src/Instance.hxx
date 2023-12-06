// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include "Listener.hxx"
#include "io/Logger.hxx"
#include "event/Loop.hxx"
#include "event/ShutdownListener.hxx"
#include "event/systemd/Watchdog.hxx"
#include "lua/State.hxx"
#include "lua/ValuePtr.hxx"

#include <forward_list>

namespace Lua { class Value; }

class Instance {
	EventLoop event_loop;
	ShutdownListener shutdown_listener;

	Systemd::Watchdog systemd_watchdog{event_loop};

	Lua::State lua_state;

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

	void AddListener(UniqueSocketDescriptor &&fd,
			 size_t max_size,
			 Lua::ValuePtr &&handler) noexcept;

	void AddListener(SocketAddress address,
			 size_t max_size,
			 Lua::ValuePtr &&handler);

	/**
	 * Listen for incoming connections on sockets passed by systemd
	 * (systemd socket activation).
	 */
	void AddSystemdListener(size_t max_size, Lua::ValuePtr &&handler);

	void Check();

private:
	void ShutdownCallback() noexcept;
};
