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

#include <boost/intrusive/list.hpp>

struct Action;

class QmqpRelayConnection final :
    public boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::auto_unlink>>,
    public NetstringServer, ConnectSocketHandler,
    NetstringClientHandler {

    lua_State *const L;
    Lua::ValuePtr handler;
    Logger logger;

    NetstringGenerator generator;

    char received_buffer[256];

    /**
     * The #LuaMail that is going to be sent once we've connected to
     * the outgoing QMQP server.
     */
    Lua::Value outgoing_mail;

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
         outgoing_mail(_L),
         connect(event_loop, *this),
         client(event_loop, 256, *this) {}

    static void Register(lua_State *L);

protected:
    void Do(const Action &action);
    void Exec(const Action &action);
    void OnConnect(int out_fd, int in_fd);
    void OnResponse(const void *data, size_t size);

    void OnRequest(AllocatedArray<uint8_t> &&payload) override;
    void OnError(std::exception_ptr ep) override;
    void OnDisconnect() override;

private:
    /* virtual methods from class ConnectSocketHandler */
    void OnSocketConnectSuccess(SocketDescriptor &&fd) override;
    void OnSocketConnectError(std::exception_ptr ep) override;

    /* virtual methods from class NetstringClientHandler */
    void OnNetstringResponse(AllocatedArray<uint8_t> &&payload) override;
    void OnNetstringError(std::exception_ptr error) override;
};

#endif
