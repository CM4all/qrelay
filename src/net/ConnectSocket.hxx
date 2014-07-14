/*
 * A class that connects to a SocketAddress.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef CONNECT_SOCKET_HXX
#define CONNECT_SOCKET_HXX

#include "SocketDescriptor.hxx"
#include "event/Event.hxx"

#include <functional>

struct sockaddr;
class Error;
class SocketAddress;

class ConnectSocket {
    SocketDescriptor fd;

    Event event;

    std::function<void(SocketDescriptor)> on_connect;
    std::function<void(Error &&)> on_error;

public:
    ConnectSocket();
    ~ConnectSocket();

    template<typename T>
    void OnConnect(T &&t) {
        on_connect = std::forward<T>(t);
    }

    template<typename T>
    void OnError(T &&t) {
        on_error = std::forward<T>(t);
    }

    bool Connect(SocketAddress address);

private:
    void OnEvent(evutil_socket_t, short events);
};

#endif
