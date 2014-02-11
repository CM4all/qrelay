/*
 * A netstring input buffer.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef NETSTRING_OUTPUT_HXX
#define NETSTRING_OUTPUT_HXX

#include "net/SendBuffer.hxx"

#include <cstddef>
#include <cassert>
#include <cstdint>

class Error;

class NetstringOutput {
    MultiSendBuffer buffers;

    char header_buffer[32];

public:
    NetstringOutput() = default;
    NetstringOutput(const void *data, size_t size);

    typedef MultiSendBuffer::Result Result;

    Result Send(int fd, Error &error) {
        return buffers.Send(fd, error);
    }
};

#endif
