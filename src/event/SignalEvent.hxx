/*
 * C++ wrappers for libevent.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef SIGNAL_EVENT_HXX
#define SIGNAL_EVENT_HXX

#include <functional>

#include <event.h>

class SignalEvent {
    struct event event;

    const std::function<void()> handler;

public:
    SignalEvent(int sig, std::function<void()> _handler)
        :handler(_handler) {
        ::evsignal_set(&event, sig, Callback, this);
        ::evsignal_add(&event, nullptr);
    }

    ~SignalEvent() {
        Delete();
    }

    void Delete() {
        ::evsignal_del(&event);
    }

private:
    static void Callback(int fd, short event, void *ctx);
};

#endif
