/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef TEMPLATE_SERVER_SOCKET_HXX
#define TEMPLATE_SERVER_SOCKET_HXX

#include "ServerSocket.hxx"

#include <tuple>

struct sockaddr;
class Error;
class SocketAddress;

template<class C, std::size_t i>
struct ApplyTuple {
    template<typename T, typename... P>
    static C *Create(int fd, T &&tuple, P&&... params) {
        return ApplyTuple<C, i - 1>::Create(fd,
                                            std::forward<T>(tuple),
                                            std::get<i - 1>(std::forward<T>(tuple)),
                                            std::forward<P>(params)...);
    }
};

template<class C>
struct ApplyTuple<C, 0> {
    template<typename T, typename... P>
    static C *Create(int fd, T &&, P&&... params) {
        return new C(std::forward<P>(params)..., fd);
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
    TemplateServerSocket(P&&... _params)
        :params(std::forward<P>(_params)...) {}

protected:
    virtual void OnAccept(int fd) override {
        CreateConnection(fd);
    };

private:
    C *CreateConnection(int fd) {
        return ApplyTuple<C, std::tuple_size<Tuple>::value>::template Create<Tuple &>(fd, params);
    }
};

#endif
