/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QRELAY_LMAIL_HXX
#define QRELAY_LMAIL_HXX

struct lua_State;
struct QmqpMail;

void
RegisterLuaMail(lua_State *L);

QmqpMail *
NewLuaMail(lua_State *L, QmqpMail &&src);

QmqpMail *
CheckLuaMail(lua_State *L, int idx);

#endif
