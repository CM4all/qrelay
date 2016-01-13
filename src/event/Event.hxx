/*
 * C++ wrappers for libevent.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef EVENT_HXX
#define EVENT_HXX

#include <inline/compiler.h>

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

    gcc_pure
    evutil_socket_t GetFd() const {
        return event_get_fd(&event);
    }

    gcc_pure
    short GetEvents() const {
        return event_get_events(&event);
    }

    gcc_pure
    event_callback_fn GetCallback() const {
        return event_get_callback(&event);
    }

    gcc_pure
    void *GetCallbackArg() const {
        return event_get_callback_arg(&event);
    }

    void Set(evutil_socket_t fd, short mask,
             event_callback_fn callback, void *ctx) {
        ::event_set(&event, fd, mask, callback, ctx);
    }

    bool Add(const struct timeval *timeout=nullptr) {
        return ::event_add(&event, timeout) == 0;
    }

    bool Add(const struct timeval &timeout) {
        return Add(&timeout);
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

    void MakeActive(short events) {
        event_active(&event, events, 0);
    }

    gcc_pure
    bool IsPending(short events) const {
        return ::event_pending(&event, events, nullptr);
    }

    gcc_pure
    bool IsTimerPending() const {
        return IsPending(EV_TIMEOUT);
    }
};

#endif