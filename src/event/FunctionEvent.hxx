/*
 * C++ wrappers for libevent.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef FUNCTION_EVENT_HXX
#define FUNCTION_EVENT_HXX

#include "Event.hxx"

#include <functional>

#include <event.h>

class FunctionEvent {
    Event event;

    const std::function<void(int fd, short event)> handler;

public:
    FunctionEvent(std::function<void(int fd, short event)> _handler)
        :handler(_handler) {
        Set(-1, 0);
    }

    ~FunctionEvent() {
        Delete();
    }

    void Set(int fd, short mask) {
        event.Set(fd, mask, Callback, this);
    }

    void Add(const struct timeval *timeout=nullptr) {
        event.Add(timeout);
    }

    void SetAdd(int fd, short mask, const struct timeval *timeout=nullptr) {
        Set(fd, mask);
        Add(timeout);
    }

    void SetTimer() {
        event.SetTimer(Callback, this);
    }

    void SetAddTimer(const struct timeval &timeout) {
        SetTimer();
        event.Add(&timeout);
    }

    void SetSignal(int sig) {
        event.SetSignal(sig, Callback, this);
    }

    void SetAddSignal(int sig) {
        SetSignal(sig);
        event.Add(nullptr);
    }

    void Delete() {
        event.Delete();
    }

    bool IsPending(short events) const {
        return event.IsPending(events);
    }

    bool IsTimerPending() const {
        return event.IsTimerPending();
    }

private:
    static void Callback(int fd, short event, void *ctx);
};

#endif
