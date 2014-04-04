/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef NETSTRING_CLIENT_HXX
#define NETSTRING_CLIENT_HXX

#include "Event.hxx"
#include "NetstringGenerator.hxx"
#include "NetstringInput.hxx"
#include "io/MultiWriteBuffer.hxx"

#include <list>
#include <functional>

template<typename T> struct ConstBuffer;

/**
 * A client that sends a netstring
 * (http://cr.yp.to/proto/netstrings.txt) and receives another
 * netstring.
 */
class NetstringClient final {
    int out_fd, in_fd;

    Event event;

    NetstringGenerator generator;
    MultiWriteBuffer write;

    NetstringInput input;

    std::function<void(const void *, size_t)> on_response;
    std::function<void(Error &&)> on_error;

public:
    NetstringClient(size_t max_size);
    ~NetstringClient();

    template<typename T>
    void OnResponse(T &&t) {
        on_response = std::forward<T>(t);
    }

    template<typename T>
    void OnError(T &&t) {
        on_error = std::forward<T>(t);
    }

    void Request(int _out_fd, int _in_fd,
                 std::list<ConstBuffer<void>> &&data);

private:
    void OnEvent(short events);
};

#endif
