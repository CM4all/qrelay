/*
 * A netstring input buffer.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "SendBuffer.hxx"
#include "util/Error.hxx"

#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

SendBuffer::Result
SendBuffer::Send(int fd, Error &error)
{
    size_t size = end - buffer;

    ssize_t nbytes = send(fd, buffer, size, MSG_DONTWAIT);
    if (nbytes < 0) {
        switch (errno) {
        case EAGAIN:
        case EINTR:
            return Result::MORE;

        default:
            error.SetErrno("Failed to send");
            return Result::ERROR;
        }
    }

    buffer += size;
    assert(buffer <= end);

    return buffer == end
        ? Result::FINISHED
        : Result::MORE;
}

MultiSendBuffer::Result
MultiSendBuffer::Send(int fd, Error &error)
{
    while (true) {
        assert(i < n);
        SendBuffer &buffer = buffers[i];
        switch (Result result = buffer.Send(fd, error)) {
        case Result::MORE:
        case Result::ERROR:
            return result;

        case Result::FINISHED:
            ++i;
            if (i == n)
                return result;
        }
    }
}
