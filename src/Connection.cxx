// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "Connection.hxx"
#include "Instance.hxx"
#include "RemoteRelay.hxx"
#include "ExecRelay.hxx"
#include "RawExecRelay.hxx"
#include "MutableMail.hxx"
#include "LMail.hxx"
#include "Action.hxx"
#include "LAction.hxx"
#include "net/AllocatedSocketAddress.hxx"
#include "net/UniqueSocketDescriptor.hxx"
#include "lua/PushLambda.hxx"
#include "lua/Util.hxx"
#include "lua/Error.hxx"
#include "util/ScopeExit.hxx"
#include "util/SpanCast.hxx"
#include "util/Compiler.h"

#include <fmt/format.h>

using std::string_view_literals::operator""sv;

using namespace Lua;

static std::string
MakeLoggerDomain(const struct ucred &cred, SocketAddress)
{
	if (cred.pid < 0)
		return "connection";

	return fmt::format("pid={} uid={}", cred.pid, cred.uid);
}

QmqpRelayConnection::QmqpRelayConnection(Instance &_instance,
					 size_t max_size,
					 Lua::ValuePtr _handler,
					 const RootLogger &parent_logger,
					 UniqueSocketDescriptor &&_fd,
					 SocketAddress address)
	:NetstringServer(_instance.GetEventLoop(), std::move(_fd), max_size),
	 instance(_instance),
	 peer_cred(GetSocket().GetPeerCredentials()),
	 handler(std::move(_handler)),
	 logger(parent_logger, MakeLoggerDomain(peer_cred, address).c_str()),
	 auto_close(handler->GetState()),
	 thread(handler->GetState()),
	 relay_timeout(_instance.GetEventLoop(), BIND_THIS_METHOD(OnRelayTimeout)) {}

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
		if (SendResponse("Dmalformed input"sv))
			delete this;
		return;
	}

	/* create a new thread for the handler coroutine */
	const auto L = thread.CreateThread(*this);

	handler->Push(L);

	NewLuaMail(L, auto_close,
		   std::move(mail), peer_cred);

	Resume(L, 1);
}

static MutableMail &
SetActionMail(lua_State *L, Lua::Ref &dest, int action_idx)
{
	PushLuaActionMail(L, action_idx);
	auto &mail = CastLuaMail(L, -1);
	dest = {L, Pop{}};
	return mail;
}

inline void
QmqpRelayConnection::DoConnect(const Action &action, const MutableMail &mail)
{
	if (action.timeout.count() > 0)
		relay_timeout.Schedule(action.timeout);

	auto *relay = new RemoteRelay(GetEventLoop(),
				      mail, AssembleHeaders(mail),
				      *this);
	relay_operation = ToDeletePointer(relay);

	relay->Start(action.connect);
}

inline void
QmqpRelayConnection::DoExec(const Action &action, const MutableMail &mail)
{
	if (action.timeout.count() > 0)
		relay_timeout.Schedule(action.timeout);

	auto *relay = new ExecRelay(GetEventLoop(),
				    mail, AssembleHeaders(mail),
				    *this);
	relay_operation = ToDeletePointer(relay);

	relay->Start(action);
}

inline void
QmqpRelayConnection::DoRawExec(const Action &action, const MutableMail &mail)
{
	if (action.timeout.count() > 0)
		relay_timeout.Schedule(action.timeout);

	auto *relay = new RawExecRelay(GetEventLoop(),
				       mail, AssembleHeaders(mail),
				       *this);
	relay_operation = ToDeletePointer(relay);

	relay->Start(action);
}

inline void
QmqpRelayConnection::Do(lua_State *L, const Action &action, int action_idx)
{
	switch (action.type) {
	case Action::Type::UNDEFINED:
		assert(false);
		gcc_unreachable();

	case Action::Type::DISCARD:
		if (SendResponse("Kdiscarded"sv))
			delete this;
		break;

	case Action::Type::REJECT:
		if (SendResponse("Drejected"sv))
			delete this;
		break;

	case Action::Type::CONNECT:
		DoConnect(action, SetActionMail(L, outgoing_mail, action_idx));
		break;

	case Action::Type::EXEC:
		DoExec(action, SetActionMail(L, outgoing_mail, action_idx));
		break;

	case Action::Type::EXEC_RAW:
		DoRawExec(action, SetActionMail(L, outgoing_mail, action_idx));
		break;
	}
}

std::list<std::span<const std::byte>>
QmqpRelayConnection::AssembleHeaders(const MutableMail &mail) noexcept
{
	std::list<std::span<const std::byte>> list;

	if (peer_cred.pid >= 0) {
		char *end = fmt::format_to(received_buffer,
					   "Received: from PID={} UID={} with QMQP\r\n",
					   peer_cred.pid, peer_cred.uid);
		list.emplace_front(AsBytes(std::string_view{received_buffer, end}));
	}

	for (const auto &i : mail.headers)
		list.emplace_front(std::as_bytes(std::span{i}));

	return list;
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
QmqpRelayConnection::OnRelayTimeout() noexcept
{
	logger(1, "timeout");

	if (SendResponse("Ztimeout"sv))
		delete this;
}

void
QmqpRelayConnection::OnRelayResponse(std::string_view response) noexcept
{
	if (SendResponse(response))
		delete this;
}

void
QmqpRelayConnection::OnRelayError(std::string_view response,
				  std::exception_ptr error) noexcept
{
	logger(1, error);
	if (SendResponse(response))
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

	if (SendResponse("Zscript failed"sv))
		delete this;
}
