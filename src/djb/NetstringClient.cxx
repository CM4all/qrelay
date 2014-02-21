/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "NetstringClient.hxx"
#include "NetstringError.hxx"
#include "net/Error.hxx"
#include "util/HugeAllocator.hxx"
#include "util/Error.hxx"

#include <unistd.h>
#include <string.h>
#include <errno.h>

static constexpr timeval send_timeout{10, 0};
static constexpr timeval recv_timeout{60, 0};
static constexpr timeval busy_timeout{5, 0};

NetstringClient::NetstringClient(size_t max_size)
    :fd(-1), event([this](int, short events){ OnEvent(events); }),
     input(max_size) {}

NetstringClient::~NetstringClient()
{
    if (fd >= 0) {
        event.Delete();
        close(fd);
    }
}

void
NetstringClient::Request(int _fd, const void *data, size_t size)
{
    assert(fd < 0);
    assert(on_response);
    assert(on_error);
    assert(_fd >= 0);

    fd = _fd;
    output = NetstringOutput(data, size);
    event.SetAdd(fd, EV_WRITE|EV_TIMEOUT|EV_PERSIST, &send_timeout);
}

void
NetstringClient::OnEvent(short events)
{
    if (events & EV_TIMEOUT) {
        on_error(Error(timeout_domain, "Connect timeout"));
    } else if (events & EV_WRITE) {
        Error error;
        switch (output.Write(fd, error)) {
        case NetstringOutput::Result::MORE:
            event.Add(&send_timeout);
            break;

        case NetstringOutput::Result::ERROR:
            on_error(std::move(error));
            break;

        case NetstringOutput::Result::FINISHED:
            event.Delete();
            event.SetAdd(fd, EV_READ|EV_TIMEOUT|EV_PERSIST, &recv_timeout);
            break;
        }
    } else if (events & EV_READ) {
        Error error;
        switch (input.Receive(fd, error)) {
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
