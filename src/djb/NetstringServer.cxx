/*
 * A server that receives netstrings
 * (http://cr.yp.to/proto/netstrings.txt) from its clients and
 * responds with another netstring.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "NetstringServer.hxx"
#include "NetstringError.hxx"
#include "util/HugeAllocator.hxx"
#include "util/Error.hxx"

#include <unistd.h>
#include <string.h>
#include <errno.h>

static constexpr timeval busy_timeout{5, 0};
static constexpr timeval idle_timeout{20, 0};

NetstringServer::NetstringServer(int _fd)
    :fd(_fd), event([this](int, short events){ OnEvent(events); }),
     input(16 * 1024 * 1024) {
    event.SetAdd(fd, EV_READ|EV_TIMEOUT|EV_PERSIST, &busy_timeout);
}

NetstringServer::~NetstringServer()
{
    event.Delete();
    close(fd);
}

bool
NetstringServer::Send(const void *data, size_t size, int flags)
{
    ssize_t nbytes = send(fd, data, size, flags);
    if (gcc_likely(nbytes == (ssize_t)size))
        return true;

    if (gcc_likely(nbytes < 0)) {
        switch (errno) {
        case ECONNRESET:
            OnDisconnect();
            return false;

        default:
            {
                Error error;
                error.SetErrno("send() failed");
                OnError(std::move(error));
                return false;
            }
        }
    }

    OnError(Error(netstring_domain, "short write"));
    return false;
}

bool
NetstringServer::SendResponse(const void *data, size_t size)
{
    char header[32];
    sprintf(header, "%zu:", size);

    if (!Send(header, strlen(header), MSG_DONTWAIT|MSG_MORE) ||
        !Send(data, size, MSG_DONTWAIT|MSG_MORE) ||
        !Send(",", 1, MSG_DONTWAIT))
        return false;

    event.Add(&idle_timeout);
    return true;
}

void
NetstringServer::OnEvent(short events)
{
    if (events & EV_TIMEOUT) {
        OnDisconnect();
        return;
    }

    Error error;
    NetstringInput::Result result = input.Receive(fd, error);
    switch (result) {
    case NetstringInput::Result::MORE:
        event.Add(&busy_timeout);
        break;

    case NetstringInput::Result::ERROR:
        OnError(std::move(error));
        break;

    case NetstringInput::Result::CLOSED:
        OnDisconnect();
        break;

    case NetstringInput::Result::FINISHED:
        event.Delete();
        OnRequest(input.GetValue(), input.GetSize());
        break;
    }

    return;
}
