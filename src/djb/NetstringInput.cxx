/*
 * A netstring input buffer.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "NetstringInput.hxx"
#include "NetstringError.hxx"
#include "util/HugeAllocator.hxx"
#include "util/Error.hxx"
#include "util/Domain.hxx"

#include <unistd.h>
#include <string.h>
#include <errno.h>

NetstringInput::~NetstringInput()
{
    if (value_buffer != nullptr)
        HugeFree(value_buffer, max_size);
}

static constexpr bool
IsDigit(char ch)
{
    return ch >= '0' && ch <= '9';
}

static bool
OnlyDigits(const char *p, size_t size)
{
    for (size_t i = 0; i != size; ++i)
        if (!IsDigit(p[i]))
            return false;

    return true;
}

inline NetstringInput::Result
NetstringInput::ReceiveHeader(int fd, Error &error)
{
    ssize_t nbytes = read(fd, header_buffer + header_position,
                          sizeof(header_buffer) - header_position);
    if (nbytes < 0) {
        switch (errno) {
        case EAGAIN:
        case EINTR:
            return Result::MORE;

        case ECONNRESET:
            return Result::CLOSED;

        default:
            error.SetErrno("read() failed");
            return Result::ERROR;
        }
    }

    if (nbytes == 0)
        return Result::CLOSED;

    header_position += nbytes;

    char *colon = (char *)memchr(header_buffer + 1, ':', header_position - 1);
    if (colon == nullptr) {
        if (header_position == sizeof(header_buffer) ||
            !OnlyDigits(header_buffer, header_position)) {
            error.Set(netstring_domain, "Malformed netstring");
            return Result::ERROR;
        }

        return Result::MORE;
    }

    *colon = 0;
    char *endptr;
    value_size = strtoul(header_buffer, &endptr, 10);
    if (endptr != colon) {
        error.Set(netstring_domain, "Malformed netstring");
        return Result::ERROR;
    }

    if (value_size >= max_size) {
        error.Format(netstring_domain, "Netstring is too large: %zu",
                     value_size);
        return Result::ERROR;
    }

    state = State::VALUE;
    value_position = 0;

    if (value_buffer == nullptr) {
        value_buffer = (uint8_t *)HugeAllocate(max_size);
        if (value_buffer == nullptr) {
            error.SetErrno();
            return Result::ERROR;
        }
    }

    size_t vbytes = header_position - (colon - header_buffer) - 1;
    memcpy(value_buffer, colon + 1, vbytes);
    return ValueData(vbytes, error);
}

NetstringInput::Result
NetstringInput::ValueData(size_t nbytes, Error &error)
{
    assert(state == State::VALUE);

    value_position += nbytes;

    if (value_position > value_size) {
        if (value_buffer[value_size] != ',' ||
            value_position > value_size + 1) {
            error.Set(netstring_domain, "Malformed netstring");
            return Result::ERROR;
        }

        state = State::FINISHED;
        return Result::FINISHED;
    }

    return Result::MORE;
}

inline NetstringInput::Result
NetstringInput::ReceiveValue(int fd, Error &error)
{
    ssize_t nbytes = read(fd, value_buffer + value_position,
                          value_size + 1 - value_position);
    if (nbytes < 0) {
        switch (errno) {
        case EAGAIN:
        case EINTR:
            return Result::MORE;

        case ECONNRESET:
            return Result::CLOSED;

        default:
            error.SetErrno("read() failed");
            return Result::ERROR;
        }
    }

    if (nbytes == 0)
        return Result::CLOSED;

    return ValueData(nbytes, error);
}

NetstringInput::Result
NetstringInput::Receive(int fd, Error &error)
{
    switch (state) {
    case State::FINISHED:
        if (value_buffer != nullptr)
            HugeDiscard(value_buffer, max_size);

        state = State::HEADER;
        header_position = 0;
        /* fall through */

    case State::HEADER:
        return ReceiveHeader(fd, error);

    case State::VALUE:
        return ReceiveValue(fd, error);
    }

    gcc_unreachable();
}
