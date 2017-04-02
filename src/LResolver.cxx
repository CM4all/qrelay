/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "LResolver.hxx"
#include "LAddress.hxx"
#include "lua/Util.hxx"
#include "net/Resolver.hxx"
#include "net/AddressInfo.hxx"

extern "C" {
#include <lauxlib.h>
}

#include <string.h>

static int
l_qmqp_resolve(lua_State *L)
{
    if (lua_gettop(L) != 1)
        return luaL_error(L, "Invalid parameter count");

    if (!lua_isstring(L, 1))
        luaL_argerror(L, 1, "string expected");

    const char *s = lua_tostring(L, 1);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    AddressInfo ai;

    try {
        ai = Resolve(s, 628, &hints);
    } catch (const std::exception &e) {
        return luaL_error(L, e.what());
    }

    NewLuaAddress(L, std::move(ai.front()));
    return 1;
}

void
RegisterLuaResolver(lua_State *L)
{
    Lua::SetGlobal(L, "qmqp_resolve", l_qmqp_resolve);
}
