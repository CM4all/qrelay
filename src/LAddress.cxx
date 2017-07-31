/*
 * author: Max Kellermann <mk@cm4all.com>
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
