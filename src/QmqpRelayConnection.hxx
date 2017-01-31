/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QMQP_RELAY_CONNECTION_HXX
#define QMQP_RELAY_CONNECTION_HXX

#include "djb/NetstringServer.hxx"
#include "djb/NetstringGenerator.hxx"
#include "djb/NetstringClient.hxx"
#include "net/ConnectSocket.hxx"
#include "util/ConstBuffer.hxx"
#include "Logger.hxx"
#include "Config.hxx"

#include <list>

struct Config;
class Error;

class QmqpRelayConnection final : public NetstringServer, ConnectSocketHandler {
    const Config &config;
    Logger logger;

    std::list<ConstBuffer<void>> request;
    NetstringGenerator generator;
    ConstBuffer<void> tail;

    char received_buffer[256];

    ConnectSocket connect;
    NetstringClient client;

public:
    QmqpRelayConnection(const Config &_config, const Logger &parent_logger,
                        EventLoop &event_loop,
                        int _fd)
        :NetstringServer(_fd),
         config(_config),
         logger(parent_logger, "connection"),
         connect(event_loop, *this),
         client(256) {}

protected:
    void Exec(const Config::Action &action);
    void OnConnect(SocketDescriptor out_fd, SocketDescriptor in_fd);
    void OnResponse(const void *data, size_t size);

    virtual void OnRequest(void *data, size_t size) override;
    virtual void OnError(Error &&error) override;
    virtual void OnDisconnect() override;

private:
    /* virtual methods from class ConnectSocketHandler */
    void OnSocketConnectSuccess(SocketDescriptor &&fd) override;
    void OnSocketConnectError(Error &&error) override;
};

#endif
