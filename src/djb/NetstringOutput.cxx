/*
 * A netstring input buffer.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "NetstringOutput.hxx"

NetstringOutput::NetstringOutput(const void *data, size_t size)
{
    buffers.Push(header_buffer, sprintf(header_buffer, "%zu:", size));
    buffers.Push(data, size);
    buffers.Push(",", 1);
}
