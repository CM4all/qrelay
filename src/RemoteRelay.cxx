// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#include "RemoteRelay.hxx"
#include "Handler.hxx"
#include "net/UniqueSocketDescriptor.hxx"

using std::string_view_literals::operator""sv;

RemoteRelay::RemoteRelay(EventLoop &event_loop, const QmqpMail &mail,
			 std::list<std::span<const std::byte>> &&additional_headers,
			 RelayHandler &_handler) noexcept
	:BasicRelay(event_loop, mail, std::move(additional_headers), _handler),
	 connect(event_loop, *this)
{
}

void
RemoteRelay::OnSocketConnectSuccess(UniqueSocketDescriptor s) noexcept
{
	FileDescriptor fd = s.Release().ToFileDescriptor();
	client.Request(fd, fd, std::move(request));
}

void
RemoteRelay::OnSocketConnectError(std::exception_ptr error) noexcept
{
	handler.OnRelayError("Zconnect failed"sv,
			     std::move(error));
}
