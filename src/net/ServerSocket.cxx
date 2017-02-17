/*
 * A socket that accepts incoming connections.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "ServerSocket.hxx"
#include "StaticSocketAddress.hxx"
#include "AllocatedSocketAddress.hxx"
#include "system/Error.hxx"

#include <daemon/log.h>

#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

ServerSocket::ServerSocket(EventLoop &event_loop)
    :event(event_loop, BIND_THIS_METHOD(OnEvent))
{
}

ServerSocket::~ServerSocket()
{
    if (fd.IsDefined())
        event.Delete();
}

void
ServerSocket::Listen(SocketDescriptor &&_fd)
{
    assert(!fd.IsDefined());
    assert(_fd.IsDefined());

    fd = std::move(_fd);
    event.Set(fd.Get(), EV_READ|EV_PERSIST);
    event.Add();
}

static SocketDescriptor
Listen(const SocketAddress address)
{
    if (address.GetFamily() == AF_UNIX) {
        const struct sockaddr_un *sun = (const struct sockaddr_un *)address.GetAddress();
        if (sun->sun_path[0] != '\0')
            /* delete non-abstract socket files before reusing them */
            unlink(sun->sun_path);
    }

    SocketDescriptor fd;
    if (!fd.Create(address.GetFamily(),
                   SOCK_STREAM|SOCK_CLOEXEC|SOCK_NONBLOCK,
                   0))
        throw MakeErrno("Failed to create socket");

    if (!fd.SetReuseAddress(true))
        throw MakeErrno("Failed to set SO_REUSEADDR");

    if (address.IsV6Any())
        fd.SetV6Only(false);

    if (!fd.Bind(address))
        throw MakeErrno("Failed to bind");

    switch (address.GetFamily()) {
    case AF_INET:
    case AF_INET6:
        fd.SetTcpFastOpen();
        break;

    case AF_LOCAL:
        fd.SetBoolOption(SOL_SOCKET, SO_PASSCRED, true);
        break;
    }

    if (listen(fd.Get(), 64) < 0)
        throw MakeErrno("Failed to listen");

    return fd;
}

void
ServerSocket::Listen(const SocketAddress address)
{
    assert(!fd.IsDefined());

    fd = ::Listen(address);
    event.Set(fd.Get(), EV_READ|EV_PERSIST);
    event.Add();
}

void
ServerSocket::ListenPath(const char *path)
{
    assert(!fd.IsDefined());

    AllocatedSocketAddress address;
    address.SetLocal(path);

    Listen(address);
}

StaticSocketAddress
ServerSocket::GetLocalAddress() const
{
    return fd.GetLocalAddress();
}

void
ServerSocket::OnEvent(unsigned)
{
    StaticSocketAddress remote_address;
    auto remote_fd = fd.Accept(remote_address);
    if (!remote_fd.IsDefined()) {
        daemon_log(1, "accept() failed: %s\n", strerror(errno));
        return;
    }

    OnAccept(std::move(remote_fd), remote_address);
}
