// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include <string_view>

struct ucred;
struct lua_State;
struct MutableMail;
namespace Lua { class AutoCloseList; }

void
RegisterLuaMail(lua_State *L);

/**
 * @param L the lua_State on whose stack the new object will be pushed
 */
MutableMail *
NewLuaMail(lua_State *L,
	   Lua::AutoCloseList &auto_close,
	   MutableMail &&src, const struct ucred &peer_cred);

MutableMail &
CastLuaMail(lua_State *L, int idx);
