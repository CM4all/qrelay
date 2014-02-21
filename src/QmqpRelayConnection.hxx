/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QMQP_RELAY_CONNECTION_HXX
#define QMQP_RELAY_CONNECTION_HXX

#include "djb/NetstringServer.hxx"
#include "djb/NetstringClient.hxx"
#include "net/ConnectSocket.hxx"
#include "Logger.hxx"
#include "Config.hxx"

struct Config;
class Error;

class QmqpRelayConnection final : public NetstringServer {
    const Config &config;
    Logger logger;

    ConnectSocket connect;
    NetstringClient client;

public:
    QmqpRelayConnection(const Config &_config, const Logger &parent_logger,
                        int _fd)
        :NetstringServer(_fd),
         config(_config),
         logger(parent_logger, "connection"),
         client(256) {}

protected:
    void Exec(const Config::Action &action, void *data, size_t size);
    void OnConnect(int out_fd, int in_fd, const void *data, size_t size);
    void OnResponse(const void *data, size_t size);

    virtual void OnRequest(void *data, size_t size) override;
    virtual void OnError(Error &&error) override;
    virtual void OnDisconnect() override;
};

#endif
