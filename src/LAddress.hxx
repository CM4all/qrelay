/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QRELAY_LADDRESS_HXX
#define QRELAY_LADDRESS_HXX

struct lua_State;
class AllocatedSocketAddress;

AllocatedSocketAddress
GetLuaAddress(lua_State *L, int idx);

#endif
