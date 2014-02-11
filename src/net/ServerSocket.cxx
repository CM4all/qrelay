/*
 * A socket that accepts incoming connections.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "ServerSocket.hxx"
#include "SocketAddress.hxx"
#include "util/Error.hxx"

#include <daemon/log.h>

#include <unistd.h>
#include <string.h>
#include <errno.h>

ServerSocket::ServerSocket()
    :fd(-1), event([this](int, short){ OnEvent(); })
{
}

ServerSocket::~ServerSocket()
{
    if (fd >= 0) {
        event.Delete();
        close(fd);
    }
}

static int
Listen(const SocketAddress &address, Error &error)
{
    int fd = socket(address.GetFamily(), SOCK_STREAM, 0);
    if (fd < 0) {
        error.SetErrno("Failed to create socket");
        return -1;
    }

    if (bind(fd, address, address.GetSize()) < 0) {
        error.SetErrno("Failed to bind");
        close(fd);
        return -1;
    }

    if (listen(fd, 64) < 0) {
        error.SetErrno("Failed to listen");
        close(fd);
        return -1;
    }

    return fd;
}

bool
ServerSocket::Listen(const SocketAddress &address, Error &error)
{
    assert(fd < 0);

    fd = ::Listen(address, error);
    if (fd < 0)
        return false;

    event.SetAdd(fd, EV_READ|EV_PERSIST);
    return true;
}

bool
ServerSocket::ListenPath(const char *path, Error &error)
{
    assert(fd < 0);

    unlink(path);

    SocketAddress address;
    address.SetLocal(path);

    return Listen(address, error);
}

void
ServerSocket::OnEvent()
{
    sockaddr_storage address;
    socklen_t address_size = sizeof(address);

    int result = accept4(fd, (sockaddr *)&address, &address_size,
                         SOCK_CLOEXEC|SOCK_NONBLOCK);
    if (result < 0) {
        daemon_log(1, "accept() failed: %s\n", strerror(errno));
        return;
    }

    OnAccept(result);
}
