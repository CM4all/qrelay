// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "LAction.hxx"
#include "Action.hxx"
#include "lua/Class.hxx"

struct LAction : Action {
	LAction(lua_State *L, int mail_idx) {
		// fenv.mail = mail
		lua_newtable(L);
		lua_pushvalue(L, mail_idx);
		lua_setfield(L, -2, "mail");
		lua_setfenv(L, -2);
	}

	static void PushMail(lua_State *L, int action_idx) {
		lua_getfenv(L, action_idx);
		lua_getfield(L, -1, "mail");
		lua_replace(L, -2);
	}
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
	return LuaAction::New(L, L, mail_idx);
}

Action *
CheckLuaAction(lua_State *L, int idx)
{
	return LuaAction::Check(L, idx);
}

void
PushLuaActionMail(lua_State *L, int idx)
{
	LAction::PushMail(L, idx);
}
