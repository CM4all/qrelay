/*
 * A class that connects to a SocketAddress.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "ConnectSocket.hxx"
#include "SocketDescriptor.hxx"
#include "SocketAddress.hxx"
#include "system/Error.hxx"

#include <daemon/log.h>

#include <stdexcept>

#include <unistd.h>
#include <string.h>
#include <errno.h>

static constexpr timeval connect_timeout{10, 0};

void
ConnectSocketHandler::OnSocketConnectTimeout()
{
    /* default implementation falls back to OnSocketConnectError() */
    OnSocketConnectError(std::make_exception_ptr(std::runtime_error("Connect timeout")));
}

ConnectSocket::ConnectSocket(EventLoop &_event_loop,
                             ConnectSocketHandler &_handler)
    :handler(_handler),
     event(_event_loop, BIND_THIS_METHOD(OnEvent))
{
}

ConnectSocket::~ConnectSocket()
{
    if (fd.IsDefined()) {
        event.Delete();
        fd.Close();
    }
}

static SocketDescriptor
Connect(const SocketAddress address)
{
    SocketDescriptor fd;
    if (!fd.Create(address.GetFamily(),
                   SOCK_STREAM|SOCK_CLOEXEC|SOCK_NONBLOCK,
                   0))
        throw MakeErrno("Failed to create socket");

    if (!fd.Connect(address) && errno != EINPROGRESS)
        throw MakeErrno("Failed to connect");

    return fd;
}

bool
ConnectSocket::Connect(const SocketAddress address)
{
    assert(!fd.IsDefined());

    try {
        fd = ::Connect(address);
    } catch (...) {
        handler.OnSocketConnectError(std::current_exception());
        return false;
    }

    event.Set(fd.Get(), EV_WRITE|EV_TIMEOUT);
    event.Add(connect_timeout);
    return true;
}

void
ConnectSocket::OnEvent(short events)
{
    if (events & EV_TIMEOUT) {
        handler.OnSocketConnectTimeout();
        return;
    }

    int s_err = fd.GetError();
    if (s_err != 0) {
        handler.OnSocketConnectError(std::make_exception_ptr(MakeErrno(s_err, "Failed to connect")));
        return;
    }

    handler.OnSocketConnectSuccess(std::exchange(fd, SocketDescriptor()));
}
