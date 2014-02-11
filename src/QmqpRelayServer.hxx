/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QMQP_RELAY_SERVER_HXX
#define QMQP_RELAY_SERVER_HXX

#include "Logger.hxx"
#include "QmqpRelayConnection.hxx"
#include "net/ServerSocket.hxx"

struct Config;

class QmqpRelayServer final : public ServerSocket {
    const Config &config;
    Logger logger;

public:
    QmqpRelayServer(const Config &_config, const Logger &_logger)
        :config(_config), logger(_logger) {}

protected:
    virtual void OnAccept(int fd) override {
        new QmqpRelayConnection(config, logger, fd);
    }
};

#endif
