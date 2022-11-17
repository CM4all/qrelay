/*
 * Copyright 2014-2021 CM4all GmbH
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
#include "system/Error.hxx"
#include "net/AllocatedSocketAddress.hxx"
#include "net/UniqueSocketDescriptor.hxx"
#include "lua/PushLambda.hxx"
#include "lua/Util.hxx"
#include "lua/Error.hxx"
#include "io/UniqueFileDescriptor.hxx"
#include "util/ScopeExit.hxx"

#include <list>

#include <stdio.h>
#include <unistd.h>

using namespace Lua;

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

QmqpRelayConnection::QmqpRelayConnection(size_t max_size,
					 Lua::ValuePtr _handler,
					 const RootLogger &parent_logger,
					 EventLoop &event_loop,
					 UniqueSocketDescriptor &&_fd,
					 SocketAddress address)
	:NetstringServer(event_loop, std::move(_fd), max_size),
	 peer_cred(GetSocket().GetPeerCredentials()),
	 handler(std::move(_handler)),
	 logger(parent_logger, MakeLoggerDomain(peer_cred, address).c_str()),
	 thread(handler->GetState()),
	 connect(event_loop, *this),
	 client(event_loop, 256, *this) {}

QmqpRelayConnection::~QmqpRelayConnection() noexcept
{
	thread.Cancel();
}

void
QmqpRelayConnection::Register(lua_State *L)
{
	RegisterLuaAction(L);
	RegisterLuaMail(L);
}

void
QmqpRelayConnection::OnRequest(AllocatedArray<std::byte> &&payload)
{
	MutableMail mail(std::move(payload));
	if (!mail.Parse()) {
		if (SendResponse("Dmalformed input"))
			delete this;
		return;
	}

	/* create a new thread for the handler coroutine */
	const auto L = thread.CreateThread(*this);

	handler->Push(L);

	NewLuaMail(L, std::move(mail), peer_cred);

	Resume(L, 1);
}

static void
SetActionMail(lua_State *L, Lua::Ref &dest, int action_idx)
{
	PushLuaActionMail(L, action_idx);
	dest = {L, Pop{}};
}

inline void
QmqpRelayConnection::Do(lua_State *L, const Action &action, int action_idx)
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
		SetActionMail(L, outgoing_mail, action_idx);
		connect.Connect(action.connect, std::chrono::seconds(20));
		break;

	case Action::Type::EXEC:
		Exec(L, action, action_idx);
		break;
	}
}

inline void
QmqpRelayConnection::Exec(lua_State *L, const Action &action, int action_idx)
try {
	assert(action.type == Action::Type::EXEC);
	assert(!action.exec.empty());

	UniqueFileDescriptor stdin_r, stdin_w, stdout_r, stdout_w;

	if (!UniqueFileDescriptor::CreatePipeNonBlock(stdin_r, stdin_w))
		throw MakeErrno("pipe() failed");

	if (!UniqueFileDescriptor::CreatePipeNonBlock(stdout_r, stdout_w))
		throw MakeErrno("pipe() failed");

	pid_t pid = fork();
	if (pid < 0)
		throw MakeErrno("fork() failed");

	if (pid == 0) {
		stdin_r.CheckDuplicate(FileDescriptor{STDIN_FILENO});
		stdout_w.CheckDuplicate(FileDescriptor{STDOUT_FILENO});

		/* disable O_NONBLOCK */
		FileDescriptor{STDIN_FILENO}.SetBlocking();
		FileDescriptor{STDOUT_FILENO}.SetBlocking();

		char *argv[Action::MAX_EXEC + 1];

		unsigned n = 0;
		for (const auto &i : action.exec)
			argv[n++] = const_cast<char *>(i.c_str());

		argv[n] = nullptr;

		execv(argv[0], argv);
		_exit(1);
	}

	stdin_r.Close();
	stdout_w.Close();

	SetActionMail(L, outgoing_mail, action_idx);
	OnConnect(stdin_w.Release(), stdout_r.Release());
} catch (...) {
	logger(1, std::current_exception());

	if (SendResponse("Zinternal server error"))
		delete this;
}

static MutableMail &
CastMail(lua_State *L, const Lua::Ref &ref)
{
	ref.Push(L);
	AtScopeExit(L) { lua_pop(L, 1); };
	return CastLuaMail(L, -1);
}

void
QmqpRelayConnection::OnConnect(FileDescriptor out_fd, FileDescriptor in_fd)
{
	const auto L = GetMainState();
	auto &mail = CastMail(L, outgoing_mail);

	std::list<std::span<const std::byte>> request;
	request.push_back(std::as_bytes(std::span{mail.message}));

	if (peer_cred.pid >= 0) {
		std::size_t length = sprintf(received_buffer,
					     "Received: from PID=%u UID=%u with QMQP\r\n",
					     unsigned(peer_cred.pid), unsigned(peer_cred.uid));
		request.emplace_front(std::as_bytes(std::span{received_buffer, length}));
	}

	for (const auto &i : mail.headers)
		request.emplace_front(std::as_bytes(std::span{i}));

	generator(request, true);
	request.emplace_back(std::as_bytes(std::span{sender_header(mail.sender.size())}));
	request.emplace_back(std::as_bytes(std::span{mail.sender}));
	request.push_back(std::as_bytes(std::span{mail.tail}));

	client.Request(out_fd, in_fd, std::move(request));
}

void
QmqpRelayConnection::OnError(std::exception_ptr ep) noexcept
{
	logger(1, ep);
	delete this;
}

void
QmqpRelayConnection::OnDisconnect() noexcept
{
	delete this;
}

void
QmqpRelayConnection::OnSocketConnectSuccess(UniqueSocketDescriptor _fd) noexcept
{
	const auto connection_fd = _fd.Release().ToFileDescriptor();
	OnConnect(connection_fd, connection_fd);
}

void
QmqpRelayConnection::OnSocketConnectError(std::exception_ptr ep) noexcept
{
	logger(1, ep);
	if (SendResponse("Zconnect failed"))
		delete this;
}

void
QmqpRelayConnection::OnNetstringResponse(AllocatedArray<std::byte> &&payload) noexcept
{
	if (SendResponse(payload))
		delete this;
}

void
QmqpRelayConnection::OnNetstringError(std::exception_ptr) noexcept
{
	if (SendResponse("Zrelay failed"))
		delete this;
}

void
QmqpRelayConnection::OnLuaFinished(lua_State *L) noexcept
try {
	const ScopeCheckStack check_thread_stack(L);

	auto *action = CheckLuaAction(L, -1);
	if (action == nullptr)
		throw std::runtime_error("Wrong return type from Lua handler");

	Do(L, *action, -1);
} catch (...) {
	OnLuaError(L, std::current_exception());
}

void
QmqpRelayConnection::OnLuaError(lua_State *, std::exception_ptr e) noexcept
{
	logger(1, e);

	if (SendResponse("Zscript failed"))
		delete this;
}
