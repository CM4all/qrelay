/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "LAction.hxx"
#include "Action.hxx"
#include "lua/Class.hxx"

static constexpr char lua_action_class[] = "qrelay.action";
typedef Lua::Class<Action, lua_action_class> LuaAction;

void
RegisterLuaAction(lua_State *L)
{
    LuaAction::Register(L);
    lua_pop(L, 1);
}

Action *
NewLuaAction(lua_State *L)
{
    return LuaAction::New(L);
}

Action *
CheckLuaAction(lua_State *L, int idx)
{
    return LuaAction::Check(L, idx);
}
