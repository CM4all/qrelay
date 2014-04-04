/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef WRITE_BUFFER_HXX
#define WRITE_BUFFER_HXX

#include <cstddef>
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

#endif
