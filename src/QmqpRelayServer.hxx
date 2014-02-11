/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QMQP_RELAY_SERVER_HXX
#define QMQP_RELAY_SERVER_HXX

#include "Logger.hxx"
#include "QmqpRelayConnection.hxx"
#include "ServerSocket.hxx"

class QmqpRelayServer final : public ServerSocket {
    Logger logger;

public:
    QmqpRelayServer(const Logger &_logger):logger(_logger) {}

protected:
    virtual void OnAccept(int fd) override {
        new QmqpRelayConnection(logger, fd);
    }
};

#endif
