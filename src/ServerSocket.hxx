/*
 * A socket that accepts incoming connections.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef SERVER_SOCKET_HXX
#define SERVER_SOCKET_HXX

#include "Event.hxx"

struct sockaddr;
class Error;

class ServerSocket {
    int fd;

    Event event;

public:
    ServerSocket();
    ~ServerSocket();

    bool Listen(const sockaddr *address, size_t size, Error &error);
    bool ListenPath(const char *path, Error &error);

protected:
    virtual void OnAccept(int fd) = 0;

private:
    void OnEvent();
};

#endif
