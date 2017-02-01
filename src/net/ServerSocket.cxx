/*
 * A socket that accepts incoming connections.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "ServerSocket.hxx"
#include "StaticSocketAddress.hxx"
#include "util/Error.hxx"

#include <daemon/log.h>

#include <unistd.h>
#include <string.h>
#include <errno.h>

ServerSocket::ServerSocket(EventLoop &event_loop)
    :fd(-1), event(event_loop, BIND_THIS_METHOD(OnEvent))
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
Listen(const SocketAddress address, Error &error)
{
    int fd = socket(address.GetFamily(),
                    SOCK_STREAM|SOCK_CLOEXEC|SOCK_NONBLOCK,
                    0);
    if (fd < 0) {
        error.SetErrno("Failed to create socket");
        return -1;
    }

    const int reuse = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                   (const char *)&reuse, sizeof(reuse)) < 0) {
        error.SetErrno("Failed to set SO_REUSEADDR");
        close(fd);
        return -1;
    }

    if (bind(fd, address.GetAddress(), address.GetSize()) < 0) {
        error.SetErrno("Failed to bind");
        close(fd);
        return -1;
    }

    if (listen(fd, 64) < 0) {
        error.SetErrno("Failed to listen");
        close(fd);
        return -1;
    }

    setsockopt(fd, SOL_SOCKET, SO_PASSCRED,
               (const char *)&reuse, sizeof(reuse));

    return fd;
}

bool
ServerSocket::Listen(const SocketAddress address, Error &error)
{
    assert(fd < 0);

    fd = ::Listen(address, error);
    if (fd < 0)
        return false;

    event.Set(fd, EV_READ|EV_PERSIST);
    event.Add();
    return true;
}

bool
ServerSocket::ListenPath(const char *path, Error &error)
{
    assert(fd < 0);

    unlink(path);

    StaticSocketAddress address;
    address.SetLocal(path);

    return Listen(address, error);
}

void
ServerSocket::OnEvent(short)
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
