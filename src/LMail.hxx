// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#ifndef QRELAY_LMAIL_HXX
#define QRELAY_LMAIL_HXX

struct ucred;
struct lua_State;
struct MutableMail;

void
RegisterLuaMail(lua_State *L);

/**
 * @param L the lua_State on whose stack the new object will be pushed
 * @param main_L the lua_State which is used to destruct Lua values
 * owned by the new object
 */
MutableMail *
NewLuaMail(lua_State *L, lua_State *main_L,
	   MutableMail &&src, const struct ucred &peer_cred);

MutableMail &
CastLuaMail(lua_State *L, int idx);

#endif
