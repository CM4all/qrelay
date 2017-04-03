/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QRELAY_LRESOLVER_HXX
#define QRELAY_LRESOLVER_HXX

struct lua_State;

void
RegisterLuaResolver(lua_State *L);

void
UnregisterLuaResolver(lua_State *L);

#endif
