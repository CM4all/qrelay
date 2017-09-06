/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QRELAY_LMAIL_HXX
#define QRELAY_LMAIL_HXX

struct lua_State;
struct MutableMail;
class SocketDescriptor;

void
RegisterLuaMail(lua_State *L);

MutableMail *
NewLuaMail(lua_State *L, MutableMail &&src, SocketDescriptor fd);

MutableMail &
CastLuaMail(lua_State *L, int idx);

#endif
