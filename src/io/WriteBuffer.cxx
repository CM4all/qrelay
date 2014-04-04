/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "WriteBuffer.hxx"
#include "util/Error.hxx"

#include <unistd.h>
#include <errno.h>

WriteBuffer::Result
WriteBuffer::Write(int fd, Error &error)
{
    ssize_t nbytes = write(fd, buffer, GetSize());
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
