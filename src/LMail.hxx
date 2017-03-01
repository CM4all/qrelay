/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QRELAY_LMAIL_HXX
#define QRELAY_LMAIL_HXX

struct lua_State;
struct MutableMail;

void
RegisterLuaMail(lua_State *L);

MutableMail *
NewLuaMail(lua_State *L, MutableMail &&src, int fd);

MutableMail *
CheckLuaMail(lua_State *L, int idx);

#endif
