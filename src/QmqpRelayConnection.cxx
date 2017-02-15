/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "QmqpRelayConnection.hxx"
#include "Mail.hxx"
#include "system/Error.hxx"
#include "net/Resolver.hxx"
#include "net/AddressInfo.hxx"
#include "net/AllocatedSocketAddress.hxx"
#include "lua/Util.hxx"
#include "lua/Error.hxx"
#include "util/OstreamException.hxx"

extern "C" {
#include <lauxlib.h>
}

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>

static constexpr struct luaL_reg mail_methods [] = {
    {"connect", QmqpRelayConnection::ConnectMethod},
    {"discard", QmqpRelayConnection::DiscardMethod},
    {"reject", QmqpRelayConnection::RejectMethod},
    {"exec", QmqpRelayConnection::ExecMethod},
    {nullptr, nullptr}
};

void
QmqpRelayConnection::Register(lua_State *L)
{
    luaL_newmetatable(L, "qrelay.mail");
    Lua::SetTable(L, -3, "__index", Index);
    lua_pop(L, 1);
}

int
QmqpRelayConnection::Index(lua_State *L)
{
    if (lua_gettop(L) != 2)
        return luaL_error(L, "Invalid parameters");

    if (!lua_isstring(L, 2))
        luaL_argerror(L, 2, "string expected");

    const char *name = lua_tostring(L, 2);

    for (const auto *i = mail_methods; i->name != nullptr; ++i) {
        if (strcmp(i->name, name) == 0) {
            Lua::Push(L, i->func);
            return 1;
        }
    }

    return luaL_error(L, "Unknown attribute");
}

QmqpRelayConnection &
QmqpRelayConnection::Check(lua_State *L, int idx)
{
    auto d = (QmqpRelayConnection **)luaL_checkudata(L, idx, "qrelay.mail");
    luaL_argcheck(L, d != nullptr, idx, "`qrelay.mail' expected");
    return **d;
}

static AllocatedSocketAddress
GetLuaAddress(lua_State *L, int idx)
{
    if (!lua_isstring(L, idx))
        luaL_argerror(L, idx, "address expected");

    const char *s = lua_tostring(L, idx);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    const auto ai = Resolve(s, 628, &hints);
    return AllocatedSocketAddress(ai.front());
}

int
QmqpRelayConnection::ConnectMethod(lua_State *L)
{
    if (lua_gettop(L) != 2)
      return luaL_error(L, "Invalid parameters");

    auto &c = Check(L, 1);
    auto &action = c.handler_action;
    if (action.IsDefined())
      return luaL_error(L, "Action already defined");

    auto address = GetLuaAddress(L, 2);

    action.type = Action::Type::CONNECT;
    action.connect = std::move(address);
    return 0;
}

int
QmqpRelayConnection::DiscardMethod(lua_State *L)
{
    if (lua_gettop(L) != 1)
      return luaL_error(L, "Invalid parameters");

    auto &c = Check(L, 1);
    auto &action = c.handler_action;
    if (action.IsDefined())
      return luaL_error(L, "Action already defined");

    action.type = Action::Type::DISCARD;
    return 0;
}

int
QmqpRelayConnection::RejectMethod(lua_State *L)
{
    if (lua_gettop(L) != 1)
      return luaL_error(L, "Invalid parameters");

    auto &c = Check(L, 1);
    auto &action = c.handler_action;
    if (action.IsDefined())
      return luaL_error(L, "Action already defined");

    action.type = Action::Type::REJECT;
    return 0;
}

int
QmqpRelayConnection::ExecMethod(lua_State *L)
{
    if (lua_gettop(L) < 2)
      return luaL_error(L, "Not enough parameters");

    std::list<std::string> l;
    const unsigned n = lua_gettop(L);
    for (unsigned i = 2; i <= n; ++i) {
        if (!lua_isstring(L, i))
            luaL_argerror(L, i, "string expected");

        l.emplace_back(lua_tostring(L, i));
    }

    auto &c = Check(L, 1);
    auto &action = c.handler_action;
    if (action.IsDefined())
      return luaL_error(L, "Action already defined");

    action.type = Action::Type::EXEC;
    action.exec = std::move(l);
    return 0;
}

void
QmqpRelayConnection::OnRequest(void *data, size_t size)
{
    assert(request.empty());

    QmqpMail mail;
    if (!mail.Parse(ConstBuffer<char>::FromVoid({data, size}))) {
        if (SendResponse("Dmalformed input"))
            delete this;
        return;
    }

    tail = mail.tail.ToVoid();
    request.push_back(mail.message.ToVoid());

    handler->Push();

    auto d = (QmqpRelayConnection **)lua_newuserdata(L, sizeof(QmqpRelayConnection **));
    *d = this;

    luaL_getmetatable(L, "qrelay.mail");
    lua_setmetatable(L, -2);

    if (lua_pcall(L, 1, 0, 0))
        throw Lua::PopError(L);

    if (!handler_action.IsDefined())
        throw std::runtime_error("Lua handler did not set an action");

    Do(handler_action);
}

void
QmqpRelayConnection::Do(const Action &action)
{
    switch (action.type) {
    case Action::Type::UNDEFINED:
        assert(false);
        gcc_unreachable();

    case Action::Type::DISCARD:
        if (SendResponse("Kdiscarded"))
            delete this;
        break;

    case Action::Type::REJECT:
        if (SendResponse("Drejected"))
            delete this;
        break;

    case Action::Type::CONNECT:
        connect.Connect(action.connect);
        break;

    case Action::Type::EXEC:
        Exec(action);
        break;
    }
}

void
QmqpRelayConnection::Exec(const Action &action)
{
    assert(action.type == Action::Type::EXEC);
    assert(!action.exec.empty());

    int stdin_pipe[2], stdout_pipe[2];

    if (pipe2(stdin_pipe, O_CLOEXEC|O_NONBLOCK) < 0) {
        OnError(std::make_exception_ptr(MakeErrno("pipe() failed")));
        return;
    }

    if (pipe2(stdout_pipe, O_CLOEXEC|O_NONBLOCK) < 0) {
        const int e = errno;
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        OnError(std::make_exception_ptr(MakeErrno(e, "pipe() failed")));
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        const int e = errno;
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        OnError(std::make_exception_ptr(MakeErrno(e, "fork() failed")));
        return;
    }

    if (pid == 0) {
        dup2(stdin_pipe[0], 0);
        dup2(stdout_pipe[1], 1);

        /* disable O_NONBLOCK */
        fcntl(0, F_SETFL, fcntl(0, F_GETFL) & ~O_NONBLOCK);
        fcntl(1, F_SETFL, fcntl(1, F_GETFL) & ~O_NONBLOCK);

        char *argv[Action::MAX_EXEC + 1];

        unsigned n = 0;
        for (const auto &i : action.exec)
            argv[n++] = const_cast<char *>(i.c_str());

        argv[n] = nullptr;

        execv(argv[0], argv);
        _exit(1);
    }

    close(stdin_pipe[0]);
    close(stdout_pipe[1]);

    OnConnect(stdin_pipe[1], stdout_pipe[0]);
}

void
QmqpRelayConnection::OnConnect(int out_fd, int in_fd)
{
    client.OnResponse(std::bind(&QmqpRelayConnection::OnResponse, this,
                                std::placeholders::_1,
                                std::placeholders::_2));
    client.OnError([this](std::exception_ptr){
            if (SendResponse("Zrelay failed"))
                delete this;
        });

    struct ucred cred;
    socklen_t len = sizeof (cred);
    if (getsockopt(GetFD(), SOL_SOCKET, SO_PEERCRED, &cred, &len) == 0) {
        int length = sprintf(received_buffer,
                             "Received: from PID=%u UID=%u with QMQP\r\n",
                             unsigned(cred.pid), unsigned(cred.uid));
        request.emplace_front(received_buffer, length);
    }

    generator(request, false);
    request.push_back(tail);

    client.Request(out_fd, in_fd, std::move(request));
}

void
QmqpRelayConnection::OnResponse(const void *data, size_t size)
{
    if (SendResponse(data, size))
        delete this;
}

void
QmqpRelayConnection::OnError(std::exception_ptr ep)
{
    logger(ep);
    delete this;
}

void
QmqpRelayConnection::OnDisconnect()
{
    delete this;
}

void
QmqpRelayConnection::OnSocketConnectSuccess(SocketDescriptor &&_fd)
{
    const int connection_fd = _fd.Steal();
    OnConnect(connection_fd, connection_fd);
}

void
QmqpRelayConnection::OnSocketConnectError(std::exception_ptr ep)
{
    logger(ep);
    if (SendResponse("Zconnect failed"))
        delete this;
}
