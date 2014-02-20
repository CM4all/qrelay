/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "QmqpRelayConnection.hxx"
#include "Mail.hxx"
#include "djb/NetstringParser.hxx"
#include "Config.hxx"
#include "util/ConstBuffer.hxx"
#include "util/Error.hxx"

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

    Error error;
    const Config::Action *action = config.GetAction(mail, error);
    if (action == nullptr) {
        logger(error.GetMessage());
        if (SendResponse("Drule error"))
            delete this;
        return;
    }

    switch (action->type) {
    case Config::Action::Type::UNDEFINED:
        assert(false);
        gcc_unreachable();

    case Config::Action::Type::DISCARD:
        if (SendResponse("Kdiscarded"))
            delete this;
        break;

    case Config::Action::Type::REJECT:
        if (SendResponse("Drejected"))
            delete this;
        break;

    case Config::Action::Type::CONNECT:
        connect.Connect(action->connect);
        break;
    }
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
