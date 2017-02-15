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
QmqpMail::Parse(ConstBuffer<void> __input)
{
    const auto _input = ConstBuffer<char>::FromVoid(__input);
    Range<const char *> input(_input.begin(), _input.end());

    message = ParseNetstring(input);
    if (message.IsNull())
        return false;

    tail = MakeConstBuffer<char>(input.begin() - 1, input.end());

    sender = ParseNetstring(input);
    if (sender.IsNull())
        return false;

    do {
        auto value = ParseNetstring(input);
        if (value.IsNull())
            return false;

        recipients.push_back(value);
    } while (!input.empty());

    return true;
}
