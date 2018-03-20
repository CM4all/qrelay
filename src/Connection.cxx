/*
 * Copyright 2014-2018 Content Management AG
 * All rights reserved.
 *
 * author: Max Kellermann <mk@cm4all.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Connection.hxx"
#include "MutableMail.hxx"
#include "LMail.hxx"
#include "Action.hxx"
#include "LAction.hxx"
#include "LAddress.hxx"
#include "system/Error.hxx"
#include "net/AllocatedSocketAddress.hxx"
#include "lua/Util.hxx"
#include "lua/Error.hxx"
#include "util/OstreamException.hxx"
#include "util/ScopeExit.hxx"

#include <list>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>

static std::string
MakeLoggerDomain(const struct ucred &cred, SocketAddress)
{
    if (cred.pid < 0)
        return "connection";

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "pid=%d uid=%d",
             int(cred.pid), int(cred.uid));
    return buffer;
}

QmqpRelayConnection::QmqpRelayConnection(Lua::ValuePtr _handler,
                                         const RootLogger &parent_logger,
                                         EventLoop &event_loop,
                                         UniqueSocketDescriptor &&_fd,
                                         SocketAddress address)
    :NetstringServer(event_loop, std::move(_fd)),
     peer_cred(GetSocket().GetPeerCredentials()),
     handler(std::move(_handler)),
     logger(parent_logger, MakeLoggerDomain(peer_cred, address).c_str()),
     outgoing_mail(handler->GetState()),
     connect(event_loop, *this),
     client(event_loop, 256, *this) {}

void
QmqpRelayConnection::Register(lua_State *L)
{
    RegisterLuaAction(L);
    RegisterLuaMail(L);
}

void
QmqpRelayConnection::OnRequest(AllocatedArray<uint8_t> &&payload)
{
    MutableMail mail(std::move(payload));
    if (!mail.Parse()) {
        if (SendResponse("Dmalformed input"))
            delete this;
        return;
    }

    handler->Push();

    const auto L = handler->GetState();
    NewLuaMail(L, std::move(mail), peer_cred);
    if (lua_pcall(L, 1, 1, 0))
        throw Lua::PopError(L);

    AtScopeExit(L) { lua_pop(L, 1); };

    auto *action = CheckLuaAction(L, -1);
    if (action == nullptr)
        throw std::runtime_error("Wrong return type from Lua handler");

    Do(*action);
}

/*
static const MutableMail &
GetMail(lua_State *L, const Action &action)
{
    PushLuaActionMail(action);
    AtScopeExit(L) { lua_pop(L, 1); };
    return *CheckLuaMail(L, -1);
}
*/

static void
SetActionMail(Lua::Value &dest, const Action &action)
{
    dest.Set(Lua::Lambda([&](){ PushLuaActionMail(action); }));
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
        SetActionMail(outgoing_mail, action);
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

    SetActionMail(outgoing_mail, action);
    OnConnect(stdin_pipe[1], stdout_pipe[0]);
}

static MutableMail &
CastMail(lua_State *L, const Lua::Value &value)
{
    value.Push();
    AtScopeExit(L) { lua_pop(L, 1); };
    return CastLuaMail(L, -1);
}

void
QmqpRelayConnection::OnConnect(int out_fd, int in_fd)
{
    const auto L = outgoing_mail.GetState();
    auto &mail = CastMail(L, outgoing_mail);

    std::list<ConstBuffer<void>> request;
    request.push_back(mail.message.ToVoid());

    if (peer_cred.pid >= 0) {
        int length = sprintf(received_buffer,
                             "Received: from PID=%u UID=%u with QMQP\r\n",
                             unsigned(peer_cred.pid), unsigned(peer_cred.uid));
        request.emplace_front(received_buffer, length);
    }

    for (const auto &i : mail.headers)
        request.emplace_front(i.data(), i.length());

    generator(request, false);
    request.push_back(mail.tail.ToVoid());

    client.Request(out_fd, in_fd, std::move(request));
}

void
QmqpRelayConnection::OnError(std::exception_ptr ep)
{
    logger(1, ep);
    delete this;
}

void
QmqpRelayConnection::OnDisconnect()
{
    delete this;
}

void
QmqpRelayConnection::OnSocketConnectSuccess(UniqueSocketDescriptor &&_fd)
{
    const int connection_fd = _fd.Steal();
    OnConnect(connection_fd, connection_fd);
}

void
QmqpRelayConnection::OnSocketConnectError(std::exception_ptr ep)
{
    logger(1, ep);
    if (SendResponse("Zconnect failed"))
        delete this;
}

void
QmqpRelayConnection::OnNetstringResponse(AllocatedArray<uint8_t> &&payload)
{
    if (SendResponse(&payload.front(), payload.size()))
        delete this;
}

void
QmqpRelayConnection::OnNetstringError(std::exception_ptr)
{
    if (SendResponse("Zrelay failed"))
        delete this;
}
