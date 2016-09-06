/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef INSTANCE_HXX
#define INSTANCE_HXX

#include "Logger.hxx"
#include "QmqpRelayServer.hxx"
#include "event/Loop.hxx"
#include "event/ShutdownListener.hxx"

struct Config;

class Instance {
    EventLoop event_loop;
    ShutdownListener shutdown_listener;

public:
    Logger logger;

    QmqpRelayServer qmqp_relay_server;

    explicit Instance(const Config &config);

    EventLoop &GetEventLoop() {
        return event_loop;
    }

private:
    void ShutdownCallback();
};

#endif
