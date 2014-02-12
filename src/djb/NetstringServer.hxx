/*
 * A server that receives netstrings
 * (http://cr.yp.to/proto/netstrings.txt) from its clients and
 * responds with another netstring.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef NETSTRING_SERVER_HXX
#define NETSTRING_SERVER_HXX

#include "Event.hxx"
#include "NetstringInput.hxx"

#include <cstddef>

class NetstringServer {
    const int fd;

    Event event;

    NetstringInput input;

public:
    NetstringServer(int _fd);
    ~NetstringServer();

protected:
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
    bool Send(const void *data, size_t size, int flags);

    void OnEvent(short events);
};

#endif
