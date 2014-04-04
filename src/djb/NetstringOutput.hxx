/*
 * A netstring input buffer.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef NETSTRING_OUTPUT_HXX
#define NETSTRING_OUTPUT_HXX

#include "io/MultiWriteBuffer.hxx"

#include <cstddef>

class Error;

class NetstringOutput {
    MultiWriteBuffer buffers;

    char header_buffer[32];

public:
    NetstringOutput() = default;
    NetstringOutput(const void *data, size_t size);

    typedef MultiWriteBuffer::Result Result;

    Result Write(int fd, Error &error) {
        return buffers.Write(fd, error);
    }
};

#endif
