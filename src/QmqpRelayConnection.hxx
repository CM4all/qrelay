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

#include <list>

struct Config;
struct Action;
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
                        SocketDescriptor &&_fd)
        :NetstringServer(event_loop, std::move(_fd)),
         config(_config),
         logger(parent_logger, "connection"),
         connect(event_loop, *this),
         client(event_loop, 256) {}

protected:
    void Do(const Action &action);
    void Exec(const Action &action);
    void OnConnect(int out_fd, int in_fd);
    void OnResponse(const void *data, size_t size);

    void OnRequest(void *data, size_t size) override;
    void OnError(std::exception_ptr ep) override;
    void OnDisconnect() override;

private:
    /* virtual methods from class ConnectSocketHandler */
    void OnSocketConnectSuccess(SocketDescriptor &&fd) override;
    void OnSocketConnectError(std::exception_ptr ep) override;
};

#endif
