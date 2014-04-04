/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "NetstringGenerator.hxx"
#include "util/ConstBuffer.hxx"

#include <cstddef>
#include <cstdio>

template<typename L>
static inline size_t
GetTotalSize(const L &list)
{
    size_t result = 0;
    for (const auto &i : list)
        result += i.size;
    return result;
}

void
NetstringGenerator::operator()(std::list<ConstBuffer<void>> &list)
{
    list.emplace_front(header_buffer, sprintf(header_buffer, "%zu:",
                                              GetTotalSize(list)));
    list.emplace_back(",", 1);
}
