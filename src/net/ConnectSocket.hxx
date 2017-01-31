/*
 * A class that connects to a SocketAddress.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef CONNECT_SOCKET_HXX
#define CONNECT_SOCKET_HXX

#include "SocketDescriptor.hxx"
#include "event/Event.hxx"

class Error;
class SocketAddress;

class ConnectSocketHandler {
public:
    virtual void OnSocketConnectSuccess(SocketDescriptor &&fd) = 0;
    virtual void OnSocketConnectTimeout();
    virtual void OnSocketConnectError(Error &&error) = 0;
};

class ConnectSocket {
    ConnectSocketHandler &handler;

    SocketDescriptor fd;

    Event event;

public:
    explicit ConnectSocket(ConnectSocketHandler &_handler);
    ~ConnectSocket();

    bool Connect(SocketAddress address);

private:
    void OnEvent(evutil_socket_t, short events);
};

#endif
