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
    :event(event_loop, BIND_THIS_METHOD(OnEvent))
{
}

ServerSocket::~ServerSocket()
{
    if (fd.IsDefined()) {
        event.Delete();
        fd.Close();
    }
}

static SocketDescriptor
Listen(const SocketAddress address, Error &error)
{
    SocketDescriptor fd;
    if (!fd.Create(address.GetFamily(),
                   SOCK_STREAM|SOCK_CLOEXEC|SOCK_NONBLOCK,
                   0, error))
        return SocketDescriptor();

    if (!fd.SetReuseAddress(true)) {
        error.SetErrno("Failed to set SO_REUSEADDR");
        return SocketDescriptor();
    }

    if (address.IsV6Any())
        fd.SetV6Only(false);

    if (!fd.Bind(address)) {
        error.SetErrno("Failed to bind");
        return SocketDescriptor();
    }

    switch (address.GetFamily()) {
    case AF_LOCAL:
        fd.SetBoolOption(SOL_SOCKET, SO_PASSCRED, true);
        break;
    }

    if (listen(fd.Get(), 64) < 0) {
        error.SetErrno("Failed to listen");
        return SocketDescriptor();
    }

    return fd;
}

bool
ServerSocket::Listen(const SocketAddress address, Error &error)
{
    assert(!fd.IsDefined());

    fd = ::Listen(address, error);
    if (!fd.IsDefined())
        return false;

    event.Set(fd.Get(), EV_READ|EV_PERSIST);
    event.Add();
    return true;
}

bool
ServerSocket::ListenPath(const char *path, Error &error)
{
    assert(!fd.IsDefined());

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

    int result = accept4(fd.Get(), (sockaddr *)&address, &address_size,
                         SOCK_CLOEXEC|SOCK_NONBLOCK);
    if (result < 0) {
        daemon_log(1, "accept() failed: %s\n", strerror(errno));
        return;
    }

    OnAccept(result);
}
