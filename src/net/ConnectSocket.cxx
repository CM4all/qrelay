/*
 * A class that connects to a SocketAddress.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "ConnectSocket.hxx"
#include "SocketAddress.hxx"
#include "Error.hxx"
#include "util/Error.hxx"

#include <daemon/log.h>

#include <unistd.h>
#include <string.h>
#include <errno.h>

static constexpr timeval connect_timeout{10, 0};

ConnectSocket::ConnectSocket()
    :fd(-1), event([this](int, short events){ OnEvent(events); })
{
}

ConnectSocket::~ConnectSocket()
{
    if (fd >= 0) {
        event.Delete();
        close(fd);
    }
}

static int
Connect(const SocketAddress &address, Error &error)
{
    int fd = socket(address.GetFamily(),
                    SOCK_STREAM|SOCK_CLOEXEC|SOCK_NONBLOCK,
                    0);
    if (fd < 0) {
        error.SetErrno("Failed to create socket");
        return -1;
    }

    if (connect(fd, address, address.GetSize()) < 0 && errno != EINPROGRESS) {
        error.SetErrno("Failed to connect");
        close(fd);
        return -1;
    }

    return fd;
}

bool
ConnectSocket::Connect(const SocketAddress &address)
{
    assert(fd < 0);
    assert(on_connect);
    assert(on_error);

    Error error;
    fd = ::Connect(address, error);
    if (fd < 0) {
        on_error(std::move(error));
        return false;
    }

    event.SetAdd(fd, EV_WRITE|EV_TIMEOUT, &connect_timeout);
    return true;
}

void
ConnectSocket::OnEvent(short events)
{
    if (events & EV_TIMEOUT) {
        on_error(Error(timeout_domain, "Connect timeout"));
        return;
    }

    int s_err = 0;
    socklen_t s_err_size = sizeof(s_err);

    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*)&s_err, &s_err_size) < 0)
        s_err = errno;

    if (s_err != 0) {
        Error error;
        error.SetErrno(s_err, "Failed to connect");
        on_error(std::move(error));
        return;
    }

    on_connect(fd);
    fd = -1;
}
