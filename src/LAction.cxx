/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "LAction.hxx"
#include "Action.hxx"
#include "lua/Class.hxx"
#include "lua/Value.hxx"

struct LAction : Action {
    Lua::Value mail;

    LAction(lua_State *L, Lua::StackIndex mail_idx)
        :mail(L, mail_idx) {}
};

static constexpr char lua_action_class[] = "qrelay.action";
typedef Lua::Class<LAction, lua_action_class> LuaAction;

void
RegisterLuaAction(lua_State *L)
{
    LuaAction::Register(L);
    lua_pop(L, 1);
}

Action *
NewLuaAction(lua_State *L, int mail_idx)
{
    return LuaAction::New(L, L, Lua::StackIndex(mail_idx));
}

Action *
CheckLuaAction(lua_State *L, int idx)
{
    return LuaAction::Check(L, idx);
}

void
PushLuaActionMail(const Action &_action)
{
    auto &action = (const LAction &)_action;
    action.mail.Push();
}
