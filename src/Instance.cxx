/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "Instance.hxx"

#include <stdexcept>
#include <iostream>
using std::cerr;
using std::endl;

Instance::Instance()
    :shutdown_listener(event_loop, BIND_THIS_METHOD(ShutdownCallback))
{
    shutdown_listener.Enable();
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
