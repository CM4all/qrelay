// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#pragma once

#include "BasicRelay.hxx"
#include "net/djb/NetstringGenerator.hxx"
#include "spawn/ExitListener.hxx"
#include "spawn/PidfdEvent.hxx"

#include <optional>

struct Action;

class ExecRelay final
	: BasicRelay, ExitListener
{
	AllocatedArray<std::byte> deferred_response;

	std::optional<PidfdEvent> pidfd;

public:
	using BasicRelay::BasicRelay;
	~ExecRelay() noexcept;

	bool Start(const Action &action) noexcept;

private:
	/* virtual methods from class ExitListener */
	void OnChildProcessExit(int status) noexcept override;

	/* virtual methods from class NetstringClientHandler */
	void OnNetstringResponse(AllocatedArray<std::byte> &&payload) noexcept override;
};
