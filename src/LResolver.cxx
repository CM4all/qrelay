// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#include "LResolver.hxx"
#include "lua/Util.hxx"
#include "lua/net/Resolver.hxx"
#include "net/log/Protocol.hxx"

extern "C" {
#include <lua.h>
}

#include <netdb.h>

void
RegisterLuaResolver(lua_State *L)
{
	static constexpr struct addrinfo hints{
		.ai_flags = AI_ADDRCONFIG,
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM,
	};

	Lua::PushResolveFunction(L, hints, 628);
	lua_setglobal(L, "qmqp_resolve");

	static constexpr struct addrinfo log_hints{
		.ai_flags = AI_ADDRCONFIG,
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_DGRAM,
	};

	Lua::PushResolveFunction(L, log_hints, Net::Log::DEFAULT_PORT);
	lua_setglobal(L, "log_resolve");
}

void
UnregisterLuaResolver(lua_State *L)
{
	Lua::SetGlobal(L, "qmqp_resolve", nullptr);
	Lua::SetGlobal(L, "log_resolve", nullptr);
}
