/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QMQP_RELAY_CONNECTION_HXX
#define QMQP_RELAY_CONNECTION_HXX

#include "io/Logger.hxx"
#include "event/net/ConnectSocket.hxx"
#include "event/net/djb/NetstringServer.hxx"
#include "event/net/djb/NetstringClient.hxx"
#include "net/djb/NetstringGenerator.hxx"
#include "lua/ValuePtr.hxx"
#include "util/ConstBuffer.hxx"

#include <boost/intrusive/list.hpp>

struct Action;

class QmqpRelayConnection final :
    public boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::auto_unlink>>,
    public NetstringServer, ConnectSocketHandler,
    NetstringClientHandler {

    const struct ucred peer_cred;

    Lua::ValuePtr handler;
    ChildLogger logger;

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
    QmqpRelayConnection(Lua::ValuePtr _handler,
                        const RootLogger &parent_logger,
                        EventLoop &event_loop,
                        UniqueSocketDescriptor &&_fd, SocketAddress address);

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
    void OnSocketConnectSuccess(UniqueSocketDescriptor &&fd) override;
    void OnSocketConnectError(std::exception_ptr ep) override;

    /* virtual methods from class NetstringClientHandler */
    void OnNetstringResponse(AllocatedArray<uint8_t> &&payload) override;
    void OnNetstringError(std::exception_ptr error) override;
};

#endif
