// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

struct lua_State;
struct Action;

void
RegisterLuaAction(lua_State *L);

Action *
NewLuaAction(lua_State *L);

Action *
CheckLuaAction(lua_State *L, int idx);
