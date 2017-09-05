/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QMQP_RELAY_SERVER_HXX
#define QMQP_RELAY_SERVER_HXX

#include "io/Logger.hxx"
#include "Connection.hxx"
#include "net/TemplateServerSocket.hxx"
#include "lua/ValuePtr.hxx"

typedef TemplateServerSocket<QmqpRelayConnection,
                             lua_State *, Lua::ValuePtr,
                             RootLogger, EventLoop &> QmqpRelayServer;

#endif
