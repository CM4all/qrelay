/*
 * A class that connects to a SocketAddress.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "ConnectSocket.hxx"
#include "SocketDescriptor.hxx"
#include "SocketAddress.hxx"
#include "Error.hxx"
#include "event/Callback.hxx"
#include "util/Error.hxx"

#include <daemon/log.h>

#include <unistd.h>
#include <string.h>
#include <errno.h>

static constexpr timeval connect_timeout{10, 0};

void
ConnectSocketHandler::OnSocketConnectTimeout()
{
    /* default implementation falls back to OnSocketConnectError() */
    OnSocketConnectError(Error(timeout_domain, "Connect timeout"));
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
Connect(const SocketAddress address, Error &error)
{
    SocketDescriptor fd;
    if (!fd.Create(address.GetFamily(),
                   SOCK_STREAM|SOCK_CLOEXEC|SOCK_NONBLOCK,
                   0,
                   error))
        return SocketDescriptor();

    if (!fd.Connect(address) && errno != EINPROGRESS) {
        error.SetErrno("Failed to connect");
        fd.Close();
        return SocketDescriptor();
    }

    return fd;
}

bool
ConnectSocket::Connect(const SocketAddress address)
{
    assert(!fd.IsDefined());

    Error error;
    fd = ::Connect(address, error);
    if (!fd.IsDefined()) {
        handler.OnSocketConnectError(std::move(error));
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
        Error error;
        error.SetErrno(s_err, "Failed to connect");
        handler.OnSocketConnectError(std::move(error));
        return;
    }

    handler.OnSocketConnectSuccess(std::exchange(fd, SocketDescriptor()));
}
