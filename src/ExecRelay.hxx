// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#pragma once

#include "BasicRelay.hxx"
#include "net/djb/NetstringGenerator.hxx"
#include "spawn/ExitListener.hxx"

#include <memory>

struct Action;
class ChildProcessRegistry;
class PidfdEvent;

class ExecRelay final
	: BasicRelay, ExitListener
{
	ChildProcessRegistry &child_process_registry;

	AllocatedArray<std::byte> deferred_response;

	std::unique_ptr<PidfdEvent> pidfd;

public:
	[[nodiscard]]
	ExecRelay(EventLoop &event_loop,
		  ChildProcessRegistry &_child_process_registry,
		  const QmqpMail &mail,
		  std::list<std::span<const std::byte>> &&additional_headers,
		  RelayHandler &_handler) noexcept;

	~ExecRelay() noexcept;

	bool Start(const Action &action) noexcept;

private:
	/* virtual methods from class ExitListener */
	void OnChildProcessExit(int status) noexcept override;

	/* virtual methods from class NetstringClientHandler */
	void OnNetstringResponse(AllocatedArray<std::byte> &&payload) noexcept override;
};
