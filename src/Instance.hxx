/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef INSTANCE_HXX
#define INSTANCE_HXX

#include "Logger.hxx"
#include "QmqpRelayServer.hxx"

struct Config;

class Instance {
public:
    Logger logger;

    QmqpRelayServer qmqp_relay_server;

    Instance(const Config &config)
        :qmqp_relay_server(config, logger) {}
};

#endif
