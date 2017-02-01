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

    Instance(const Config &config, EventLoop &event_loop)
        :qmqp_relay_server(event_loop, config, logger, event_loop) {}
};

#endif
