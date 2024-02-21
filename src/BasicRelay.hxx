// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include "event/net/djb/NetstringClient.hxx"
#include "net/djb/NetstringGenerator.hxx"

struct QmqpMail;
class RelayHandler;
class SocketAddress;

class BasicRelay
	: protected NetstringClientHandler
{
	NetstringGenerator generator;
	NetstringHeader sender_header;

protected:
	RelayHandler &handler;

	std::list<std::span<const std::byte>> request;

	NetstringClient client;

public:
	[[nodiscard]]
	BasicRelay(EventLoop &event_loop, const QmqpMail &mail,
		   std::list<std::span<const std::byte>> &&additional_headers,
		   RelayHandler &_handler) noexcept;

	auto &GetEventLoop() const noexcept {
		return client.GetEventLoop();
	}

protected:
	/* virtual methods from class NetstringClientHandler */
	void OnNetstringResponse(AllocatedArray<std::byte> &&payload) noexcept override;
	void OnNetstringError(std::exception_ptr error) noexcept override;
};
