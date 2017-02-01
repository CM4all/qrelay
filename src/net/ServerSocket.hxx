/*
 * A socket that accepts incoming connections.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef SERVER_SOCKET_HXX
#define SERVER_SOCKET_HXX

#include "event/SocketEvent.hxx"

struct sockaddr;
class Error;
class SocketAddress;

class ServerSocket {
    int fd;

    SocketEvent event;

public:
    explicit ServerSocket(EventLoop &event_loop);
    ~ServerSocket();

    bool Listen(SocketAddress address, Error &error);
    bool ListenPath(const char *path, Error &error);

protected:
    virtual void OnAccept(int fd) = 0;

private:
    void OnEvent(short events);
};

#endif
