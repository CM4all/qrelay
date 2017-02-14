/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef TEMPLATE_SERVER_SOCKET_HXX
#define TEMPLATE_SERVER_SOCKET_HXX

#include "ServerSocket.hxx"
#include "SocketAddress.hxx"

#include <tuple>

class SocketAddress;

template<class C, std::size_t i>
struct ApplyTuple {
    template<typename T, typename... P>
    static C *Create(SocketDescriptor &&fd, T &&tuple, P&&... params) {
        return ApplyTuple<C, i - 1>::Create(std::move(fd),
                                            std::forward<T>(tuple),
                                            std::get<i - 1>(std::forward<T>(tuple)),
                                            std::forward<P>(params)...);
    }
};

template<class C>
struct ApplyTuple<C, 0> {
    template<typename T, typename... P>
    static C *Create(SocketDescriptor &&fd, T &&, P&&... params) {
        return new C(std::forward<P>(params)..., std::move(fd));
    }
};

/**
 * A #ServerSocket implementation that creates a new instance of the
 * given class for each connection.
 */
template<typename C, typename... Params>
class TemplateServerSocket : public ServerSocket {
    typedef std::tuple<Params...> Tuple;
    Tuple params;

public:
    template<typename... P>
    TemplateServerSocket(EventLoop &event_loop, P&&... _params)
        :ServerSocket(event_loop), params(std::forward<P>(_params)...) {}

protected:
    void OnAccept(SocketDescriptor &&_fd, SocketAddress) override {
        CreateConnection(std::move(_fd));
    };

private:
    C *CreateConnection(SocketDescriptor &&_fd) {
        return ApplyTuple<C, std::tuple_size<Tuple>::value>::template Create<Tuple &>(std::move(_fd), params);
    }
};

#endif
