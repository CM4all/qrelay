/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "QmqpRelayConnection.hxx"
#include "util/Error.hxx"

void
QmqpRelayConnection::OnRequest(void *data, size_t size)
{
    SendResponse(data, size);
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
