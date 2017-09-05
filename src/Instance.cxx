/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "Instance.hxx"

#include <systemd/sd-daemon.h>

#include <stdexcept>
#include <iostream>
using std::cerr;
using std::endl;

#include <errno.h>
#include <string.h>

Instance::Instance()
    :shutdown_listener(event_loop, BIND_THIS_METHOD(ShutdownCallback))
{
    shutdown_listener.Enable();
}

void
Instance::AddSystemdQmqpRelayServer(lua_State *L,
                                    Lua::ValuePtr &&handler)
{
    int n = sd_listen_fds(true);
    if (n < 0) {
        logger(1, "sd_listen_fds() failed: ", strerror(errno));
        return;
    }

    if (n == 0) {
        logger(1, "No systemd socket");
        return;
    }

    for (unsigned i = 0; i < unsigned(n); ++i) {
        qmqp_relay_servers.emplace_front(event_loop, L,
                                         Lua::ValuePtr(handler),
                                         logger, event_loop);
        qmqp_relay_servers.front().Listen(UniqueSocketDescriptor(SD_LISTEN_FDS_START + i));
    }
}

void
Instance::Check()
{
    if (qmqp_relay_servers.empty())
        throw std::runtime_error("No QMQP listeners configured");
}

void
Instance::ShutdownCallback()
{
    cerr << "quit" << endl;
    event_loop.Break();
}
