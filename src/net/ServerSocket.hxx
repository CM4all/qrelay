/*
 * A socket that accepts incoming connections.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef SERVER_SOCKET_HXX
#define SERVER_SOCKET_HXX

#include "net/UniqueSocketDescriptor.hxx"
#include "event/SocketEvent.hxx"

class SocketAddress;

class ServerSocket {
    UniqueSocketDescriptor fd;

    SocketEvent event;

public:
    explicit ServerSocket(EventLoop &event_loop);
    ~ServerSocket();

    void Listen(UniqueSocketDescriptor &&_fd);
    void Listen(SocketAddress address);
    void ListenPath(const char *path);

    StaticSocketAddress GetLocalAddress() const;

    bool SetTcpDeferAccept(const int &seconds) {
        return fd.SetTcpDeferAccept(seconds);
    }

    void AddEvent() {
        event.Add();
    }

    void RemoveEvent() {
        event.Delete();
    }

protected:
    /**
     * A new incoming connection has been established.
     *
     * @param fd the socket owned by the callee
     */
    virtual void OnAccept(UniqueSocketDescriptor &&fd,
                          SocketAddress address) = 0;

private:
    void OnEvent(unsigned events);
};

#endif
