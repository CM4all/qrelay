/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "NetstringParser.hxx"

#include <algorithm>

#include <stdlib.h>

ConstBuffer<char>
ParseNetstring(Range<const char *> &input_r)
{
    const Range<const char *> input = input_r;

    const char *colon = std::find(input.begin(), input.end(), ':');
    if (colon == input.end())
        return nullptr;

    char *endptr;
    const size_t value_size = strtoul(input.begin(), &endptr, 10);
    if (endptr != colon)
        return nullptr;

    const char *value = colon + 1;

    const size_t header_size = value - input.begin();
    size_t remaining = input.size() - header_size;
    if (value_size >= remaining || value[value_size] != ',')
        return nullptr;

    input_r = { value + value_size + 1, input.end() };
    return { value, value_size };
}
