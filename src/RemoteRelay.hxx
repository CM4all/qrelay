// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include "BasicRelay.hxx"
#include "event/net/djb/NetstringClient.hxx"
#include "event/net/ConnectSocket.hxx"
#include "net/djb/NetstringGenerator.hxx"

class RemoteRelay final
	: BasicRelay, ConnectSocketHandler
{
	ConnectSocket connect;

public:
	[[nodiscard]]
	RemoteRelay(EventLoop &event_loop, const QmqpMail &mail,
		    std::list<std::span<const std::byte>> &&additional_headers,
		    RelayHandler &_handler) noexcept;

	bool Start(const auto &address) noexcept {
		return connect.Connect(address, std::chrono::seconds{20});
	}

private:
	/* virtual methods from class ConnectSocketHandler */
	void OnSocketConnectSuccess(UniqueSocketDescriptor fd) noexcept override;
	void OnSocketConnectError(std::exception_ptr error) noexcept override;
};
