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

#ifndef QMQP_RELAY_CONNECTION_HXX
#define QMQP_RELAY_CONNECTION_HXX

#include "io/Logger.hxx"
#include "event/net/ConnectSocket.hxx"
#include "event/net/djb/NetstringServer.hxx"
#include "event/net/djb/NetstringClient.hxx"
#include "net/djb/NetstringGenerator.hxx"
#include "lua/Ref.hxx"
#include "lua/Resume.hxx"
#include "lua/ValuePtr.hxx"
#include "util/ConstBuffer.hxx"
#include "util/IntrusiveList.hxx"

#include <sys/socket.h>

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

	NetstringGenerator generator;
	NetstringHeader sender_header;

	char received_buffer[256];

	/**
	 * The Lua thread which runs the handler coroutine.
	 */
	Lua::Value thread;

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
		return thread.GetState();
	}

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
