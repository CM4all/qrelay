/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QRELAY_LMAIL_HXX
#define QRELAY_LMAIL_HXX

struct ucred;
struct lua_State;
struct MutableMail;

void
RegisterLuaMail(lua_State *L);

MutableMail *
NewLuaMail(lua_State *L, MutableMail &&src, const struct ucred &peer_cred);

MutableMail &
CastLuaMail(lua_State *L, int idx);

#endif
