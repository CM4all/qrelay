/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef TEMPLATE_SERVER_SOCKET_HXX
#define TEMPLATE_SERVER_SOCKET_HXX

#include "ServerSocket.hxx"
#include "SocketAddress.hxx"

#include <boost/intrusive/list.hpp>

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
    static_assert(std::is_base_of<boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::auto_unlink>>,
                                  C>::value,
                  "Must use list_base_hook<auto_unlink>");

    typedef std::tuple<Params...> Tuple;
    Tuple params;

    boost::intrusive::list<C,
                           boost::intrusive::constant_time_size<false>> connections;

public:
    template<typename... P>
    TemplateServerSocket(EventLoop &event_loop, P&&... _params)
        :ServerSocket(event_loop), params(std::forward<P>(_params)...) {}

    ~TemplateServerSocket() {
        while (!connections.empty())
            delete &connections.front();
    }

protected:
    void OnAccept(SocketDescriptor &&_fd, SocketAddress) override {
        auto *c = CreateConnection(std::move(_fd));
        connections.push_front(*c);
    };

private:
    C *CreateConnection(SocketDescriptor &&_fd) {
        return ApplyTuple<C, std::tuple_size<Tuple>::value>::template Create<Tuple &>(std::move(_fd), params);
    }
};

#endif
