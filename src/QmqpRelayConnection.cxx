/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "QmqpRelayConnection.hxx"
#include "djb/NetstringParser.hxx"
#include "Config.hxx"
#include "util/ConstBuffer.hxx"
#include "util/Error.hxx"

#include <vector>

struct QmqpMail {
    ConstBuffer<char> message;
    ConstBuffer<char> tail;

    ConstBuffer<char> sender;
    std::vector<ConstBuffer<char>> recipients;

    bool Parse(ConstBuffer<char> input);
};

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

void
QmqpRelayConnection::OnRequest(void *data, size_t size)
{
    QmqpMail mail;
    if (!mail.Parse(ConstBuffer<char>::FromVoid({data, size}))) {
        if (SendResponse("Dmalformed input"))
            delete this;
        return;
    }

    connect.OnConnect(std::bind(&QmqpRelayConnection::OnConnect, this,
                                std::placeholders::_1, data, size));
    connect.OnError([this](Error &&error){
            logger(error.GetMessage());
            if (SendResponse("Zconnect failed"))
                delete this;
        });

    connect.Connect(config.connect);
}

void
QmqpRelayConnection::OnConnect(int fd, const void *data, size_t size)
{
    client.OnResponse(std::bind(&QmqpRelayConnection::OnResponse, this,
                                std::placeholders::_1,
                                std::placeholders::_2));
    client.OnError([this](Error &&error){
            if (SendResponse("Zrelay failed"))
                delete this;
        });

    client.Request(fd, data, size);
}

void
QmqpRelayConnection::OnResponse(const void *data, size_t size)
{
    if (SendResponse(data, size))
        delete this;
}

void
QmqpRelayConnection::OnError(Error &&error)
{
    logger(error.GetMessage());
    delete this;
}

void
QmqpRelayConnection::OnDisconnect()
{
    delete this;
}
