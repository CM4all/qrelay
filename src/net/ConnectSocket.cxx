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

ConnectSocket::ConnectSocket()
    :fd(-1)
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
    assert(on_connect);
    assert(on_error);

    Error error;
    fd = ::Connect(address, error);
    if (!fd.IsDefined()) {
        on_error(std::move(error));
        return false;
    }

    event.Set(fd.Get(), EV_WRITE|EV_TIMEOUT,
              MakeEventCallback(ConnectSocket, OnEvent), this);
    event.Add(connect_timeout);
    return true;
}

void
ConnectSocket::OnEvent(evutil_socket_t, short events)
{
    if (events & EV_TIMEOUT) {
        on_error(Error(timeout_domain, "Connect timeout"));
        return;
    }

    int s_err = fd.GetError();
    if (s_err != 0) {
        Error error;
        error.SetErrno(s_err, "Failed to connect");
        on_error(std::move(error));
        return;
    }

    on_connect(std::exchange(fd, SocketDescriptor()));
}
