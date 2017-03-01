/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef INSTANCE_HXX
#define INSTANCE_HXX

#include "Logger.hxx"
#include "Server.hxx"
#include "event/Loop.hxx"
#include "event/ShutdownListener.hxx"
#include "lua/ValuePtr.hxx"

#include <forward_list>

namespace Lua { class Value; }

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

    void AddQmqpRelayServer(SocketAddress address,
                            lua_State *L,
                            Lua::ValuePtr &&handler) {
        qmqp_relay_servers.emplace_front(event_loop, L, handler,
                                         logger, event_loop);
        qmqp_relay_servers.front().Listen(address);
    }

    /**
     * Listen for incoming connections on sockets passed by systemd
     * (systemd socket activation).
     */
    void AddSystemdQmqpRelayServer(lua_State *L,
                                   Lua::ValuePtr &&handler);

    void Check();

private:
    void ShutdownCallback();
};

#endif
