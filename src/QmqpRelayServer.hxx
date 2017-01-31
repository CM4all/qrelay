/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QMQP_RELAY_SERVER_HXX
#define QMQP_RELAY_SERVER_HXX

#include "Logger.hxx"
#include "QmqpRelayConnection.hxx"
#include "net/TemplateServerSocket.hxx"

struct Config;

typedef TemplateServerSocket<QmqpRelayConnection,
                             const Config &, Logger, EventLoop &> QmqpRelayServer;

#endif
