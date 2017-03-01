/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QMQP_RELAY_CONNECTION_HXX
#define QMQP_RELAY_CONNECTION_HXX

#include "Logger.hxx"
#include "djb/NetstringServer.hxx"
#include "djb/NetstringGenerator.hxx"
#include "djb/NetstringClient.hxx"
#include "net/ConnectSocket.hxx"
#include "lua/ValuePtr.hxx"
#include "util/ConstBuffer.hxx"

#include <list>

struct Action;

class QmqpRelayConnection final :
    public NetstringServer, ConnectSocketHandler,
    NetstringClientHandler {

    lua_State *const L;
    Lua::ValuePtr handler;
    Logger logger;

    std::list<ConstBuffer<void>> request;
    NetstringGenerator generator;
    ConstBuffer<void> tail;

    char received_buffer[256];

    ConnectSocket connect;
    NetstringClient client;

public:
    QmqpRelayConnection(lua_State *_L, Lua::ValuePtr _handler,
                        const Logger &parent_logger,
                        EventLoop &event_loop,
                        SocketDescriptor &&_fd)
        :NetstringServer(event_loop, std::move(_fd)),
         L(_L), handler(std::move(_handler)),
         logger(parent_logger, "connection"),
         connect(event_loop, *this),
         client(event_loop, 256, *this) {}

    static void Register(lua_State *L);

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

    /* virtual methods from class NetstringClientHandler */
    void OnNetstringResponse(ConstBuffer<void> payload) override;
    void OnNetstringError(std::exception_ptr error) override;
};

#endif
