/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "NetstringClient.hxx"
#include "NetstringError.hxx"
#include "net/Error.hxx"
#include "event/Callback.hxx"
#include "util/ConstBuffer.hxx"
#include "util/HugeAllocator.hxx"
#include "util/Error.hxx"

#include <unistd.h>
#include <string.h>
#include <errno.h>

static constexpr timeval send_timeout{10, 0};
static constexpr timeval recv_timeout{60, 0};
static constexpr timeval busy_timeout{5, 0};

NetstringClient::NetstringClient(size_t max_size)
    :out_fd(-1), in_fd(-1),
     input(max_size) {}

NetstringClient::~NetstringClient()
{
    if (out_fd >= 0 || in_fd >= 0)
        event.Delete();

    if (out_fd >= 0)
        close(out_fd);

    if (in_fd >= 0 && in_fd != out_fd)
        close(in_fd);
}

void
NetstringClient::Request(int _out_fd, int _in_fd,
                         std::list<ConstBuffer<void>> &&data)
{
    assert(in_fd < 0);
    assert(out_fd < 0);
    assert(on_response);
    assert(on_error);
    assert(_in_fd >= 0);
    assert(_out_fd >= 0);

    out_fd = _out_fd;
    in_fd = _in_fd;

    generator(data);
    for (const auto &i : data)
        write.Push(i.data, i.size);

    event.Set(out_fd, EV_WRITE|EV_TIMEOUT|EV_PERSIST,
              MakeEventCallback(NetstringClient, OnEvent), this);
    event.Add(send_timeout);
}

void
NetstringClient::OnEvent(evutil_socket_t, short events)
{
    if (events & EV_TIMEOUT) {
        on_error(Error(timeout_domain, "Connect timeout"));
    } else if (events & EV_WRITE) {
        Error error;
        switch (write.Write(out_fd, error)) {
        case MultiWriteBuffer::Result::MORE:
            event.Add(&send_timeout);
            break;

        case MultiWriteBuffer::Result::ERROR:
            on_error(std::move(error));
            break;

        case MultiWriteBuffer::Result::FINISHED:
            event.Delete();
            event.Set(in_fd, EV_READ|EV_TIMEOUT|EV_PERSIST,
                      MakeEventCallback(NetstringClient, OnEvent), this);
            event.Add(recv_timeout);
            break;
        }
    } else if (events & EV_READ) {
        Error error;
        switch (input.Receive(in_fd, error)) {
        case NetstringInput::Result::MORE:
            event.Add(&busy_timeout);
            break;

        case NetstringInput::Result::ERROR:
            on_error(std::move(error));
            break;

        case NetstringInput::Result::CLOSED:
            on_error(Error(netstring_domain, "Connection closed prematurely"));
            break;

        case NetstringInput::Result::FINISHED:
            event.Delete();
            on_response(input.GetValue(), input.GetSize());
            break;
        }
    }
}
