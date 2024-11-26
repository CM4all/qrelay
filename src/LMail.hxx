// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include <string_view>

struct lua_State;
struct MutableMail;
class SocketPeerAuth;
namespace Lua { class AutoCloseList; }

void
RegisterLuaMail(lua_State *L);

/**
 * @param L the lua_State on whose stack the new object will be pushed
 */
MutableMail *
NewLuaMail(lua_State *L,
	   Lua::AutoCloseList &auto_close,
	   MutableMail &&src, const SocketPeerAuth &peer_auth);

MutableMail &
CastLuaMail(lua_State *L, int idx);
