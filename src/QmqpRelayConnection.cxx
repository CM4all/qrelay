/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "QmqpRelayConnection.hxx"
#include "Config.hxx"
#include "util/Error.hxx"

void
QmqpRelayConnection::OnRequest(void *data, size_t size)
{
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
