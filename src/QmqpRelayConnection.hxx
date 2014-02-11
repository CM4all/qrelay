/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QMQP_RELAY_CONNECTION_HXX
#define QMQP_RELAY_CONNECTION_HXX

#include "djb/NetstringServer.hxx"
#include "Logger.hxx"

class Error;

class QmqpRelayConnection final : public NetstringServer {
    Logger logger;

public:
    QmqpRelayConnection(const Logger &parent_logger, int _fd)
        :NetstringServer(_fd),
         logger(parent_logger, "connection") {}

protected:
    virtual void OnRequest(void *data, size_t size) override;
    virtual void OnError(Error &&error) override;
    virtual void OnDisconnect() override;
};

#endif
