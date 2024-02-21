// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#ifndef QMQP_RELAY_CONNECTION_HXX
#define QMQP_RELAY_CONNECTION_HXX

#include "io/Logger.hxx"
#include "event/net/ConnectSocket.hxx"
#include "event/net/djb/NetstringServer.hxx"
#include "event/net/djb/NetstringClient.hxx"
#include "net/djb/NetstringGenerator.hxx"
#include "lua/AutoCloseList.hxx"
#include "lua/Ref.hxx"
#include "lua/Resume.hxx"
#include "lua/ValuePtr.hxx"
#include "lua/CoRunner.hxx"
#include "util/IntrusiveList.hxx"

#include <list>

#include <sys/socket.h>

struct MutableMail;
struct Action;
class FileDescriptor;

class QmqpRelayConnection final :
	public AutoUnlinkIntrusiveListHook,
	public NetstringServer, ConnectSocketHandler,
	Lua::ResumeListener,
	NetstringClientHandler {

	const struct ucred peer_cred;

	const Lua::ValuePtr handler;
	ChildLogger logger;

	Lua::AutoCloseList auto_close;

	NetstringGenerator generator;
	NetstringHeader sender_header;

	char received_buffer[256];

	/**
	 * The Lua thread which runs the handler coroutine.
	 */
	Lua::CoRunner thread;

	/**
	 * The #LuaMail that is going to be sent once we've connected to
	 * the outgoing QMQP server.
	 */
	Lua::Ref outgoing_mail;

	ConnectSocket connect;
	NetstringClient client;

public:
	QmqpRelayConnection(size_t max_size, Lua::ValuePtr _handler,
			    const RootLogger &parent_logger,
			    EventLoop &event_loop,
			    UniqueSocketDescriptor &&_fd, SocketAddress address);
	~QmqpRelayConnection() noexcept;

	static void Register(lua_State *L);

protected:
	void Do(lua_State *L, const Action &action, int action_idx);
	void Exec(lua_State *L, const Action &action, int action_idx);
	void OnConnect(FileDescriptor out_fd, FileDescriptor in_fd);
	void OnResponse(const void *data, size_t size);

	void OnRequest(AllocatedArray<std::byte> &&payload) override;
	void OnError(std::exception_ptr ep) noexcept override;
	void OnDisconnect() noexcept override;

private:
	lua_State *GetMainState() const noexcept {
		return thread.GetMainState();
	}

	/**
	 * Assemble all buffers that will be sent to the outgoing
	 * server.  This is the original mail plus headers generated
	 * by this process.
	 */
	std::list<std::span<const std::byte>> AssembleOutgoingMail(const MutableMail &mail) noexcept;

	/* virtual methods from class ConnectSocketHandler */
	void OnSocketConnectSuccess(UniqueSocketDescriptor fd) noexcept override;
	void OnSocketConnectError(std::exception_ptr ep) noexcept override;

	/* virtual methods from class NetstringClientHandler */
	void OnNetstringResponse(AllocatedArray<std::byte> &&payload) noexcept override;
	void OnNetstringError(std::exception_ptr error) noexcept override;

	/* virtual methods from class Lua::ResumeListener */
	void OnLuaFinished(lua_State *L) noexcept override;
	void OnLuaError(lua_State *L, std::exception_ptr e) noexcept override;
};

#endif
