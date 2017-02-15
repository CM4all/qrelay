/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "LAddress.hxx"
#include "net/Resolver.hxx"
#include "net/AddressInfo.hxx"
#include "net/AllocatedSocketAddress.hxx"

extern "C" {
#include <lauxlib.h>
}

#include <string.h>

AllocatedSocketAddress
GetLuaAddress(lua_State *L, int idx)
{
    if (!lua_isstring(L, idx))
        luaL_argerror(L, idx, "address expected");

    const char *s = lua_tostring(L, idx);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    const auto ai = Resolve(s, 628, &hints);
    return AllocatedSocketAddress(ai.front());
}
