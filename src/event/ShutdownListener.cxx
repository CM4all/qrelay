/*
 * Listener for shutdown signals (SIGTERM, SIGINT, SIGQUIT).
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "ShutdownListener.hxx"

#include <inline/compiler.h>

#include <signal.h>
#include <unistd.h>

inline void
ShutdownListener::SignalCallback(int)
{
    Disable();
    callback();
}

ShutdownListener::ShutdownListener(EventLoop &loop, Callback _callback)
    :event(loop, BIND_THIS_METHOD(SignalCallback)),
     callback(_callback)
{
    event.Add(SIGTERM);
    event.Add(SIGINT);
    event.Add(SIGQUIT);
}
