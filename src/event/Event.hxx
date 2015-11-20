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

    const std::function<void(int fd, short event)> handler;

public:
    Event(std::function<void(int fd, short event)> _handler)
        :handler(_handler) {
        Set(-1, 0);
    }

    ~Event() {
        Delete();
    }

    Event(const Event &other) = delete;
    Event &operator=(const Event &other) = delete;

    void Set(int fd, short mask) {
        ::event_set(&event, fd, mask, Callback, this);
    }

    void Add(const struct timeval *timeout=nullptr) {
        ::event_add(&event, timeout);
    }

    void SetAdd(int fd, short mask, const struct timeval *timeout=nullptr) {
        Set(fd, mask);
        Add(timeout);
    }

    void SetTimer() {
        ::evtimer_set(&event, Callback, this);
    }

    void SetAddTimer(const struct timeval &timeout) {
        SetTimer();
        ::event_add(&event, &timeout);
    }

    void SetSignal(int sig) {
        ::evsignal_set(&event, sig, Callback, this);
    }

    void SetAddSignal(int sig) {
        SetSignal(sig);
        ::evsignal_add(&event, nullptr);
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

private:
    static void Callback(int fd, short event, void *ctx);
};

#endif
