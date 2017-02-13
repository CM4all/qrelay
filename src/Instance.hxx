/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef INSTANCE_HXX
#define INSTANCE_HXX

#include "Logger.hxx"
#include "QmqpRelayServer.hxx"
#include "event/Loop.hxx"
#include "event/ShutdownListener.hxx"

#include <forward_list>

struct Config;

class Instance {
    EventLoop event_loop;
    ShutdownListener shutdown_listener;

    std::forward_list<QmqpRelayServer> qmqp_relay_servers;

public:
    Logger logger;

    Instance();

    EventLoop &GetEventLoop() {
        return event_loop;
    }

    void AddQmqpRelayServer(const Config &config) {
        qmqp_relay_servers.emplace_front(event_loop, config, logger,
                                         event_loop);
        qmqp_relay_servers.front().Listen(config.listen);
    }

private:
    void ShutdownCallback();
};

#endif
