/*
 * Copyright 2014-2021 CM4all GmbH
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

#include "Instance.hxx"
#include "net/SocketConfig.hxx"

extern "C" {
#include <lauxlib.h>
}

#include <systemd/sd-daemon.h>

#include <stdexcept>

#include <errno.h>
#include <string.h>

Instance::Instance()
	:shutdown_listener(event_loop, BIND_THIS_METHOD(ShutdownCallback)),
	 lua_state(luaL_newstate())
{
	shutdown_listener.Enable();
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

void
Instance::Check()
{
	if (listeners.empty())
		throw std::runtime_error("No QMQP listeners configured");
}

void
Instance::ShutdownCallback() noexcept
{
	event_loop.Break();
}
