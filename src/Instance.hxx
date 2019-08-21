/*
 * Copyright 2014-2018 Content Management AG
 * All rights reserved.
 *
 * author: Max Kellermann <mk@cm4all.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef INSTANCE_HXX
#define INSTANCE_HXX

#include "Listener.hxx"
#include "io/Logger.hxx"
#include "event/Loop.hxx"
#include "event/ShutdownListener.hxx"
#include "lua/State.hxx"
#include "lua/ValuePtr.hxx"

#include <forward_list>

namespace Lua { class Value; }

class Instance {
	EventLoop event_loop;
	ShutdownListener shutdown_listener;

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

#endif
