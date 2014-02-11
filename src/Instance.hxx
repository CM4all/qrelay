/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef INSTANCE_HXX
#define INSTANCE_HXX

#include "Logger.hxx"
#include "QmqpRelayServer.hxx"

class Instance {
public:
    Logger logger;

    QmqpRelayServer qmqp_relay_server;

    Instance()
        :qmqp_relay_server(logger) {}
};

#endif
