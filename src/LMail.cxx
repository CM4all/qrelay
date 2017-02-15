/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "LMail.hxx"
#include "Mail.hxx"
#include "LAction.hxx"
#include "Action.hxx"
#include "LAddress.hxx"
#include "lua/Class.hxx"

#include <string.h>

static constexpr char lua_mail_class[] = "qrelay.mail";
typedef Lua::Class<QmqpMail, lua_mail_class> LuaMail;

static int
NewConnectAction(lua_State *L)
{
    if (lua_gettop(L) != 2)
      return luaL_error(L, "Invalid parameters");

    auto &action = *NewLuaAction(L);
    action.type = Action::Type::CONNECT;
    action.connect = GetLuaAddress(L, 2);
    return 1;
}

static int
NewDiscardAction(lua_State *L)
{
    if (lua_gettop(L) != 1)
      return luaL_error(L, "Invalid parameters");

    auto &action = *NewLuaAction(L);
    action.type = Action::Type::DISCARD;
    return 1;
}

static int
NewRejectAction(lua_State *L)
{
    if (lua_gettop(L) != 1)
      return luaL_error(L, "Invalid parameters");

    auto &action = *NewLuaAction(L);
    action.type = Action::Type::REJECT;
    return 1;
}

static int
NewExecAction(lua_State *L)
{
    if (lua_gettop(L) < 2)
      return luaL_error(L, "Not enough parameters");

    auto &action = *NewLuaAction(L);
    action.type = Action::Type::EXEC;

    const unsigned n = lua_gettop(L);
    for (unsigned i = 2; i <= n; ++i) {
        if (!lua_isstring(L, i))
            luaL_argerror(L, i, "string expected");

        action.exec.emplace_back(lua_tostring(L, i));
    }

    return 1;
}

static constexpr struct luaL_reg mail_methods [] = {
    {"connect", NewConnectAction},
    {"discard", NewDiscardAction},
    {"reject", NewRejectAction},
    {"exec", NewExecAction},
    {nullptr, nullptr}
};

static void
PushArray(lua_State *L, const std::vector<StringView> &src)
{
    lua_createtable(L, src.size(), 0);

    int i = 1;
    for (const auto &value : src)
        Lua::SetTable(L, -3, i++, value);
}

static int
LuaMailIndex(lua_State *L)
{
    if (lua_gettop(L) != 2)
        return luaL_error(L, "Invalid parameters");

    auto &mail = *CheckLuaMail(L, 1);

    if (!lua_isstring(L, 2))
        luaL_argerror(L, 2, "string expected");

    const char *name = lua_tostring(L, 2);

    for (const auto *i = mail_methods; i->name != nullptr; ++i) {
        if (strcmp(i->name, name) == 0) {
            Lua::Push(L, i->func);
            return 1;
        }
    }

    if (strcmp(name, "sender") == 0) {
        Lua::Push(L, mail.sender);
        return 1;
    } else if (strcmp(name, "recipients") == 0) {
        PushArray(L, mail.recipients);
        return 1;
    }

    return luaL_error(L, "Unknown attribute");
}

void
RegisterLuaMail(lua_State *L)
{
    LuaMail::Register(L);
    Lua::SetTable(L, -3, "__index", LuaMailIndex);
    lua_pop(L, 1);
}

QmqpMail *
NewLuaMail(lua_State *L)
{
    return LuaMail::New(L);
}

QmqpMail *
NewLuaMail(lua_State *L, QmqpMail &&src)
{
    return LuaMail::New(L, std::move(src));
}

QmqpMail *
CheckLuaMail(lua_State *L, int idx)
{
    return LuaMail::Check(L, idx);
}
