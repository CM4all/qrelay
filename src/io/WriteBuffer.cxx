/*
 * A netstring input buffer.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "WriteBuffer.hxx"
#include "util/Error.hxx"

#include <sys/uio.h>
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

MultiWriteBuffer::Result
MultiWriteBuffer::Write(int fd, Error &error)
{
    assert(i < n);

    std::array<iovec, MAX_BUFFERS> iov;

    for (unsigned k = i; k != n; ++k) {
        iov[k].iov_base = const_cast<void *>(buffers[k].GetData());
        iov[k].iov_len = buffers[k].GetSize();
    };

    ssize_t nbytes = writev(fd, &iov[i], n - i);
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

    while (i != n) {
        WriteBuffer &b = buffers[i];
        if (size_t(nbytes) < b.GetSize()) {
            b.buffer += nbytes;
            return Result::MORE;
        }

        nbytes -= b.GetSize();
        ++i;
    }

    return Result::FINISHED;
}
