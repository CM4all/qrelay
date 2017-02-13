/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "Instance.hxx"

#include <iostream>
using std::cerr;
using std::endl;

Instance::Instance()
    :shutdown_listener(event_loop, BIND_THIS_METHOD(ShutdownCallback))
{
    shutdown_listener.Enable();
}

void
Instance::ShutdownCallback()
{
    cerr << "quit" << endl;
    event_loop.Break();
}
