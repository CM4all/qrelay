/*
 * A server that receives netstrings
 * (http://cr.yp.to/proto/netstrings.txt) from its clients and
 * responds with another netstring.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef NETSTRING_SERVER_HXX
#define NETSTRING_SERVER_HXX

#include "NetstringInput.hxx"
#include "NetstringGenerator.hxx"
#include "io/MultiWriteBuffer.hxx"
#include "event/SocketEvent.hxx"

#include <cstddef>

class NetstringServer {
    const int fd;

    SocketEvent event;

    NetstringInput input;
    NetstringGenerator generator;
    MultiWriteBuffer write;

public:
    NetstringServer(EventLoop &event_loop, int _fd);
    ~NetstringServer();

protected:
    int GetFD() const {
        return fd;
    }

    bool SendResponse(const void *data, size_t size);
    bool SendResponse(const char *data);

    /**
     * A netstring has been received.
     *
     * @param data the netstring value; for the implementation's
     * convenience, the netstring is null-terminated and writable
     */
    virtual void OnRequest(void *data, size_t size) = 0;
    virtual void OnError(Error &&error) = 0;
    virtual void OnDisconnect() = 0;

private:
    void OnEvent(short events);
};

#endif
