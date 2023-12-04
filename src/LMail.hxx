// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#ifndef QRELAY_LMAIL_HXX
#define QRELAY_LMAIL_HXX

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

#endif
