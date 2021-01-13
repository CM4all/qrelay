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

#include "LAddress.hxx"
#include "lua/Class.hxx"
#include "net/Parser.hxx"
#include "net/AllocatedSocketAddress.hxx"
#include "net/ToString.hxx"

extern "C" {
#include <lauxlib.h>
}

#include <string.h>

static constexpr char lua_address_class[] = "qrelay.address";
typedef Lua::Class<AllocatedSocketAddress, lua_address_class> LuaAddress;

static const AllocatedSocketAddress &
CastLuaAddress(lua_State *L, int idx)
{
	return LuaAddress::Cast(L, idx);
}

static int
LuaAddressToString(lua_State *L)
{
	if (lua_gettop(L) != 1)
		return luaL_error(L, "Invalid parameters");

	const auto &a = CastLuaAddress(L, 1);

	char buffer[256];
	if (!ToString(buffer, sizeof(buffer), a))
		return 0;

	Lua::Push(L, buffer);
	return 1;
}

void
RegisterLuaAddress(lua_State *L)
{
	LuaAddress::Register(L);
	Lua::SetTable(L, -3, "__tostring", LuaAddressToString);
	lua_pop(L, 1);
}

AllocatedSocketAddress *
NewLuaAddress(lua_State *L, SocketAddress src)
{
	return LuaAddress::New(L, src);
}

AllocatedSocketAddress *
NewLuaAddress(lua_State *L, AllocatedSocketAddress &&src)
{
	return LuaAddress::New(L, std::move(src));
}

AllocatedSocketAddress
GetLuaAddress(lua_State *L, int idx)
{
	if (lua_isstring(L, idx)) {
		const char *s = lua_tostring(L, idx);

		return ParseSocketAddress(s, 628, false);
	} else {
		return CastLuaAddress(L, idx);
	}
}
