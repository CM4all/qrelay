/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QRELAY_LACTION_HXX
#define QRELAY_LACTION_HXX

struct lua_State;
struct Action;

void
RegisterLuaAction(lua_State *L);

/**
 * @param mail_idx the index of the associated #MutableMail instance
 * on the Lua stack
 */
Action *
NewLuaAction(lua_State *L, int mail_idx);

Action *
CheckLuaAction(lua_State *L, int idx);

/**
 * Push the associated mail object on the Lua stack.
 */
void
PushLuaActionMail(const Action &action);

#endif
