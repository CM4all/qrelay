/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "QmqpMail.hxx"
#include "NetstringParser.hxx"
#include "util/ConstBuffer.hxx"

static inline constexpr StringView
MakeStringView(const char *begin, const char *end)
{
    return {begin, size_t(end - begin)};
}

bool
QmqpMail::Parse(ConstBuffer<void> __input)
{
    const auto _input = ConstBuffer<char>::FromVoid(__input);
    Range<const char *> input(_input.begin(), _input.end());

    message = ParseNetstring(input);
    if (message.IsNull())
        return false;

    tail = MakeStringView(input.begin() - 1, input.end());

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
