/*
 * A netstring input buffer.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef SEND_BUFFER_HXX
#define SEND_BUFFER_HXX

#include <array>
#include <cstddef>
#include <cassert>
#include <cstdint>

class Error;

class SendBuffer {
    const uint8_t *buffer, *end;

public:
    SendBuffer() = default;
    SendBuffer(const void *_buffer, size_t size)
        :buffer((const uint8_t *)_buffer), end(buffer + size) {}

    enum class Result {
        MORE,
        ERROR,
        FINISHED,
    };

    Result Send(int fd, Error &error);
};

class MultiSendBuffer {
    unsigned i, n;

    std::array<SendBuffer, 8> buffers;

public:
    MultiSendBuffer():i(0), n(0) {}

    typedef SendBuffer::Result Result;

    void Push(const void *buffer, size_t size) {
        assert(n < buffers.size());

        buffers[n++] = SendBuffer(buffer, size);
    }

    Result Send(int fd, Error &error);
};

#endif
