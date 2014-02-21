/*
 * A netstring input buffer.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "WriteBuffer.hxx"
#include "util/Error.hxx"

#include <unistd.h>
#include <errno.h>

WriteBuffer::Result
WriteBuffer::Write(int fd, Error &error)
{
    size_t size = end - buffer;

    ssize_t nbytes = write(fd, buffer, size);
    if (nbytes < 0) {
        switch (errno) {
        case EAGAIN:
        case EINTR:
            return Result::MORE;

        default:
            error.SetErrno("Failed to write");
            return Result::ERROR;
        }
    }

    buffer += nbytes;
    assert(buffer <= end);

    return buffer == end
        ? Result::FINISHED
        : Result::MORE;
}

MultiWriteBuffer::Result
MultiWriteBuffer::Write(int fd, Error &error)
{
    while (true) {
        assert(i < n);
        WriteBuffer &buffer = buffers[i];
        switch (Result result = buffer.Write(fd, error)) {
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
