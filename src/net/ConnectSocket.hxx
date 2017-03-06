/*
 * A class that connects to a SocketAddress.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef CONNECT_SOCKET_HXX
#define CONNECT_SOCKET_HXX

#include "net/SocketDescriptor.hxx"
#include "event/SocketEvent.hxx"

#include <exception>

class Error;
class SocketAddress;

class ConnectSocketHandler {
public:
    virtual void OnSocketConnectSuccess(SocketDescriptor &&fd) = 0;
    virtual void OnSocketConnectTimeout();
    virtual void OnSocketConnectError(std::exception_ptr ep) = 0;
};

class ConnectSocket {
    ConnectSocketHandler &handler;

    SocketDescriptor fd;

    SocketEvent event;

public:
    explicit ConnectSocket(EventLoop &_event_loop,
                           ConnectSocketHandler &_handler);
    ~ConnectSocket();

    bool Connect(SocketAddress address);

private:
    void OnEvent(unsigned events);
};

#endif
