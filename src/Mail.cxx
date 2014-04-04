/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "Mail.hxx"
#include "djb/NetstringParser.hxx"
#include "util/ConstBuffer.hxx"

template<typename T>
static inline constexpr ConstBuffer<T>
MakeConstBuffer(typename ConstBuffer<T>::pointer_type begin,
                typename ConstBuffer<T>::pointer_type end)
{
    return ConstBuffer<T>(begin, end - begin);
}

bool
QmqpMail::Parse(ConstBuffer<char> _input)
{
    Range<const char *> input(_input.begin(), _input.end());

    auto value = ParseNetstring(input);
    if (value.IsNull())
        return false;

    message = ConstBuffer<char>::FromVoid(value);
    tail = MakeConstBuffer<char>(input.begin() - 1, input.end());

    value = ParseNetstring(input);
    if (value.IsNull())
        return false;

    sender = ConstBuffer<char>::FromVoid(value);

    do {
        value = ParseNetstring(input);
        if (value.IsNull())
            return false;

        recipients.push_back(value);
    } while (!input.empty());

    return true;
}
