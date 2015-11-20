/*
 * C++ wrappers for libevent.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef EVENT_BASE_HXX
#define EVENT_BASE_HXX

#include <functional>

#include <event.h>

class EventBase {
    struct event_base *const event_base;

public:
    EventBase():event_base(::event_init()) {}
    ~EventBase() {
        ::event_base_free(event_base);
    }

    EventBase(const EventBase &other) = delete;
    EventBase &operator=(const EventBase &other) = delete;

    void Dispatch() {
        ::event_base_dispatch(event_base);
    }

    void Break() {
        ::event_base_loopbreak(event_base);
    }
};

#endif
