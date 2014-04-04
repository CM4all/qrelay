/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef MULTI_WRITE_BUFFER_HXX
#define MULTI_WRITE_BUFFER_HXX

#include "WriteBuffer.hxx"

#include <array>
#include <cstddef>
#include <cassert>

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
