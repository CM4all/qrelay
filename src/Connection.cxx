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
#include "net/log/Datagram.hxx"
#include "net/log/Send.hxx"
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
MakeLoggerDomain(const SocketPeerAuth &auth, SocketAddress)
{
	if (!auth.HaveCred())
		return "connection";

	return fmt::format("pid={} uid={}", auth.GetPid(), auth.GetUid());
}

QmqpRelayConnection::QmqpRelayConnection(Instance &_instance,
					 size_t max_size,
					 Lua::ValuePtr _handler,
					 const RootLogger &parent_logger,
					 UniqueSocketDescriptor &&_fd,
					 SocketAddress address)
	:NetstringServer(_instance.GetEventLoop(), std::move(_fd), max_size),
	 instance(_instance),
	 start_time(_instance.GetEventLoop().SteadyNow()),
	 peer_auth(GetSocket()),
	 handler(std::move(_handler)),
	 logger(parent_logger, MakeLoggerDomain(peer_auth, address).c_str()),
	 auto_close(handler->GetState()),
	 thread(handler->GetState()),
	 relay_timeout(_instance.GetEventLoop(), BIND_THIS_METHOD(OnRelayTimeout)) {}

QmqpRelayConnection::~QmqpRelayConnection() noexcept
{
	switch (state) {
	case State::INIT:
		/* nothing received yet, don't bother logging this */
		break;

	case State::RECEIVED:
		/* we received something, but maybe it's malformed, so
		   can't log it */
		break;

	case State::LUA:
	case State::RELAYING:
		Log("canceled"sv);
		break;

	case State::NOT_RELAYING:
		/* this should not be reachable because the connection
		   does not get destructed without calling Log() */
		break;

	case State::END:
		/* already logged */
		break;
	}

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
	assert(state == State::INIT);
	state = State::RECEIVED;

	MutableMail mail(std::move(payload));
	if (!mail.Parse()) {
		Finish("Dmalformed input"sv);
		return;
	}

	/* create a new thread for the handler coroutine */
	const auto L = thread.CreateThread(*this);

	handler->Push(L);

	mail_ptr = NewLuaMail(L, auto_close,
			      std::move(mail), peer_auth);
	lua_mail = {L, Lua::RelativeStackIndex{-1}};

	state = State::LUA;

	Resume(L, 1);
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
QmqpRelayConnection::Do(const Action &action, const MutableMail &mail)
{
	switch (action.type) {
	case Action::Type::UNDEFINED:
		assert(false);
		gcc_unreachable();

	case Action::Type::DISCARD:
		state = State::NOT_RELAYING;
		Finish("Kdiscarded"sv);
		break;

	case Action::Type::REJECT:
		state = State::NOT_RELAYING;
		Finish("Drejected"sv);
		break;

	case Action::Type::CONNECT:
		state = State::RELAYING;
		DoConnect(action, mail);
		break;

	case Action::Type::EXEC:
		state = State::RELAYING;
		DoExec(action, mail);
		break;

	case Action::Type::EXEC_RAW:
		state = State::RELAYING;
		DoRawExec(action, mail);
		break;
	}
}

std::list<std::span<const std::byte>>
QmqpRelayConnection::AssembleHeaders(const MutableMail &mail) noexcept
{
	std::list<std::span<const std::byte>> list;

	if (peer_auth.HaveCred()) {
		char *end = fmt::format_to(received_buffer,
					   "Received: from PID={} UID={} with QMQP\r\n",
					   peer_auth.GetPid(), peer_auth.GetUid());
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

	if (state > State::INIT)
		Log("client error"sv);
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
	Finish("Ztimeout"sv);
}

void
QmqpRelayConnection::OnRelayResponse(std::string_view response) noexcept
{
	Finish(response);
}

void
QmqpRelayConnection::OnRelayError(std::string_view response,
				  std::exception_ptr error) noexcept
{
	logger(1, error);
	Finish(response);
}

void
QmqpRelayConnection::OnLuaFinished(lua_State *L) noexcept
try {
	assert(state == State::LUA);

	const ScopeCheckStack check_thread_stack(L);

	auto *action = CheckLuaAction(L, -1);
	if (action == nullptr)
		throw std::runtime_error("Wrong return type from Lua handler");

	Do(*action, *mail_ptr);
} catch (...) {
	OnLuaError(L, std::current_exception());
}

void
QmqpRelayConnection::OnLuaError(lua_State *, std::exception_ptr e) noexcept
{
	assert(state == State::LUA);

	logger(1, e);
	Finish("Zscript failed"sv);
}

[[gnu::pure]]
static std::size_t
TotalSize(const std::forward_list<std::string> &list) noexcept
{
	std::size_t result = 0;
	for (const auto &i : list)
		result += i.size();
	return result;
}

void
QmqpRelayConnection::Log(std::string_view message) noexcept
{
	assert(state != State::END);

	const auto &log_socket = instance.GetLogSocket();
	if (!log_socket.IsDefined())
		/* logging is disabled */
		return;

	std::string message_buffer =
		fmt::format("from=<{}> to="sv, mail_ptr->sender);
	bool comma = false;
	for (const auto &i : mail_ptr->recipients) {
		if (comma)
			message_buffer.push_back(',');
		comma = true;
		message_buffer += fmt::format("<{}>"sv, i);
	}

	if (!message.empty()) {
		message_buffer.push_back(' ');
		message_buffer.append(message);
	}

	message = message_buffer;

	const std::size_t added_header_size = TotalSize(mail_ptr->headers);
	const uint_least64_t traffic_received = mail_ptr->buffer.size();
	const uint_least64_t traffic_sent = state >= State::RELAYING
		? traffic_received + added_header_size
		: 0;

	const auto d = Net::Log::Datagram{
		.timestamp = Net::Log::FromSystem(GetEventLoop().SystemNow()),
		.site = mail_ptr->account.empty() ? nullptr : mail_ptr->account.c_str(),
		.message = message,
		.type = Net::Log::Type::SUBMISSION,
	}.SetTraffic(traffic_received, traffic_sent)
		.SetLength(mail_ptr->message.size() + added_header_size)
		.SetDuration(std::chrono::duration_cast<Net::Log::Duration>(GetEventLoop().SteadyNow() - start_time));

	try {
		Net::Log::Send(log_socket, d);
	} catch (...) {
		PrintException(std::current_exception());
	}

	state = State::END;
}

void
QmqpRelayConnection::Finish(std::string_view response) noexcept
{
	assert(state != State::INIT && state != State::END);

	if (state != State::RECEIVED)
		Log(response.substr(1));

	if (SendResponse(response))
		delete this;
}
