/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef INSTANCE_HXX
#define INSTANCE_HXX

#include "Logger.hxx"
#include "QmqpRelayServer.hxx"
#include "event/Loop.hxx"

struct Config;

class Instance {
    EventLoop event_loop;

public:
    Logger logger;

    QmqpRelayServer qmqp_relay_server;

    explicit Instance(const Config &config)
        :qmqp_relay_server(event_loop, config, logger, event_loop) {}

    EventLoop &GetEventLoop() {
        return event_loop;
    }
};

#endif
