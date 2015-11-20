/*
 * A socket that accepts incoming connections.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef SERVER_SOCKET_HXX
#define SERVER_SOCKET_HXX

#include "event/FunctionEvent.hxx"

struct sockaddr;
class Error;
class SocketAddress;

class ServerSocket {
    int fd;

    FunctionEvent event;

public:
    ServerSocket();
    ~ServerSocket();

    bool Listen(const SocketAddress &address, Error &error);
    bool ListenPath(const char *path, Error &error);

protected:
    virtual void OnAccept(int fd) = 0;

private:
    void OnEvent();
};

#endif
