/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QRELAY_LADDRESS_HXX
#define QRELAY_LADDRESS_HXX

struct lua_State;
class SocketAddress;
class AllocatedSocketAddress;

void
RegisterLuaAddress(lua_State *L);

AllocatedSocketAddress *
NewLuaAddress(lua_State *L, SocketAddress src);

AllocatedSocketAddress *
NewLuaAddress(lua_State *L, AllocatedSocketAddress &&src);

AllocatedSocketAddress *
CheckLuaAddress(lua_State *L, int idx);

AllocatedSocketAddress
GetLuaAddress(lua_State *L, int idx);

#endif
