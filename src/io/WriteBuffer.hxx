/*
 * A netstring input buffer.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef WRITE_BUFFER_HXX
#define WRITE_BUFFER_HXX

#include <array>
#include <cstddef>
#include <cassert>
#include <cstdint>

class Error;

class WriteBuffer {
    friend class MultiWriteBuffer;

    const uint8_t *buffer, *end;

public:
    WriteBuffer() = default;
    WriteBuffer(const void *_buffer, size_t size)
        :buffer((const uint8_t *)_buffer), end(buffer + size) {}

    const void *GetData() const {
        return buffer;
    }

    size_t GetSize() const {
        return end - buffer;
    }

    enum class Result {
        MORE,
        ERROR,
        FINISHED,
    };

    Result Write(int fd, Error &error);
};

class MultiWriteBuffer {
    static constexpr size_t MAX_BUFFERS = 8;

    unsigned i, n;

    std::array<WriteBuffer, MAX_BUFFERS> buffers;

public:
    MultiWriteBuffer():i(0), n(0) {}

    typedef WriteBuffer::Result Result;

    void Push(const void *buffer, size_t size) {
        assert(n < buffers.size());

        buffers[n++] = WriteBuffer(buffer, size);
    }

    Result Write(int fd, Error &error);
};

#endif
