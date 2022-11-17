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

#include "LResolver.hxx"
#include "lua/Util.hxx"
#include "lua/Error.hxx"
#include "lua/net/SocketAddress.hxx"
#include "net/Resolver.hxx"
#include "net/AddressInfo.hxx"
#include "net/AllocatedSocketAddress.hxx"

extern "C" {
#include <lauxlib.h>
}

#include <string.h>

static int
l_qmqp_resolve(lua_State *L)
{
	if (lua_gettop(L) != 1)
		return luaL_error(L, "Invalid parameter count");

	const char *s = luaL_checkstring(L, 1);

	if (*s == '/' || *s == '@') {
		AllocatedSocketAddress address;
		address.SetLocal(s);
		Lua::NewSocketAddress(L, std::move(address));
		return 1;
	}

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	AddressInfoList ai;

	try {
		ai = Resolve(s, 628, &hints);
	} catch (...) {
		Lua::RaiseCurrent(L);
	}

	Lua::NewSocketAddress(L, std::move(ai.GetBest()));
	return 1;
}

void
RegisterLuaResolver(lua_State *L)
{
	Lua::SetGlobal(L, "qmqp_resolve", l_qmqp_resolve);
}

void
UnregisterLuaResolver(lua_State *L)
{
	Lua::SetGlobal(L, "qmqp_resolve", nullptr);
}
