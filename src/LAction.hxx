/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QRELAY_LACTION_HXX
#define QRELAY_LACTION_HXX

struct lua_State;
struct Action;

void
RegisterLuaAction(lua_State *L);

Action *
NewLuaAction(lua_State *L);

Action *
CheckLuaAction(lua_State *L, int idx);

#endif
