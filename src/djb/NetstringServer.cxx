/*
 * A server that receives netstrings
 * (http://cr.yp.to/proto/netstrings.txt) from its clients and
 * responds with another netstring.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "NetstringServer.hxx"
#include "NetstringError.hxx"
#include "util/ConstBuffer.hxx"
#include "util/HugeAllocator.hxx"
#include "util/Error.hxx"

#include <unistd.h>
#include <string.h>
#include <errno.h>

static constexpr timeval busy_timeout{5, 0};

NetstringServer::NetstringServer(EventLoop &event_loop, SocketDescriptor &&_fd)
    :fd(std::move(_fd)),
     event(event_loop, fd.Get(), EV_READ|EV_TIMEOUT|EV_PERSIST,
           BIND_THIS_METHOD(OnEvent)),
     input(16 * 1024 * 1024) {
    event.Add(busy_timeout);
}

NetstringServer::~NetstringServer()
{
    event.Delete();
}

bool
NetstringServer::SendResponse(const void *data, size_t size)
{
    std::list<ConstBuffer<void>> list{{data, size}};
    generator(list);
    for (const auto &i : list)
        write.Push(i.data, i.size);

    Error error;
    switch (write.Write(fd.Get(), error)) {
    case MultiWriteBuffer::Result::MORE:
        OnError(Error(netstring_domain, "short write"));
        return false;

    case MultiWriteBuffer::Result::ERROR:
        OnError(std::move(error));
        return false;

    case MultiWriteBuffer::Result::FINISHED:
        return true;
    }

    assert(false);
    gcc_unreachable();
}

bool
NetstringServer::SendResponse(const char *data)
{
    return SendResponse((const void *)data, strlen(data));
}

void
NetstringServer::OnEvent(short events)
{
    if (events & EV_TIMEOUT) {
        OnDisconnect();
        return;
    }

    Error error;
    NetstringInput::Result result = input.Receive(fd.Get(), error);
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
}
