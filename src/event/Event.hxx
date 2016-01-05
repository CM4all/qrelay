/*
 * C++ wrappers for libevent.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef EVENT_HXX
#define EVENT_HXX

#include <functional>

#include <event.h>

class Event {
    struct event event;

public:
    Event() = default;

    Event(evutil_socket_t fd, short mask,
          event_callback_fn callback, void *ctx) {
        Set(fd, mask, callback, ctx);
    }

    Event(const Event &other) = delete;
    Event &operator=(const Event &other) = delete;

    void Set(evutil_socket_t fd, short mask,
             event_callback_fn callback, void *ctx) {
        ::event_set(&event, fd, mask, callback, ctx);
    }

    void Add(const struct timeval *timeout=nullptr) {
        ::event_add(&event, timeout);
    }

    void SetTimer(event_callback_fn callback, void *ctx) {
        ::evtimer_set(&event, callback, ctx);
    }

    void SetSignal(int sig, event_callback_fn callback, void *ctx) {
        ::evsignal_set(&event, sig, callback, ctx);
    }

    void Delete() {
        ::event_del(&event);
    }

    bool IsPending(short events) const {
        return ::event_pending(&event, events, nullptr);
    }

    bool IsTimerPending() const {
        return IsPending(EV_TIMEOUT);
    }
};

#endif
