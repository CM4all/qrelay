// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include "spawn/ExitListener.hxx"
#include "spawn/PidfdEvent.hxx"
#include "event/PipeEvent.hxx"
#include "io/MultiWriteBuffer.hxx"

#include <array>
#include <list>
#include <optional>

struct Action;
struct QmqpMail;
class RelayHandler;

class RawExecRelay final
	: ExitListener
{
	RelayHandler &handler;

	std::optional<PidfdEvent> pidfd;

	MultiWriteBuffer request_buffer;

	PipeEvent request_pipe, response_pipe;

	std::array<std::byte, 256> response_buffer;

	/**
	 * How much of #response_buffer was filled?  This is
	 * initialized with 1 because we'll insert the QMQP code here.
	 */
	std::size_t response_fill = 1;

	int status;

public:
	RawExecRelay(EventLoop &event_loop, const QmqpMail &mail,
		     std::list<std::span<const std::byte>> &&additional_headers,
		     RelayHandler &_handler);
	~RawExecRelay() noexcept;

	auto &GetEventLoop() const noexcept {
		return request_pipe.GetEventLoop();
	}

	bool Start(const Action &action) noexcept;

private:
	bool TryWrite() noexcept;
	bool TryRead() noexcept;

	void OnRequestPipeReady(unsigned events) noexcept;
	void OnResponsePipeReady(unsigned events) noexcept;

	void Finish() noexcept;

	/* virtual methods from class ExitListener */
	void OnChildProcessExit(int status) noexcept override;
};
