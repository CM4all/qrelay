// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#include "BasicRelay.hxx"
#include "Handler.hxx"
#include "djb/QmqpMail.hxx"
#include "util/SpanCast.hxx"

using std::string_view_literals::operator""sv;

BasicRelay::BasicRelay(EventLoop &event_loop, const QmqpMail &mail,
		       std::list<std::span<const std::byte>> &&additional_headers,
		       RelayHandler &_handler) noexcept
	:handler(_handler),
	 request(std::move(additional_headers)),
	 client(event_loop, 256, *this)
{
	request.push_back(AsBytes(mail.message));

	generator(request, true);
	request.emplace_back(std::as_bytes(std::span{sender_header(mail.sender.size())}));
	request.push_back(AsBytes(mail.sender));
	request.push_back(AsBytes(mail.tail));

}

static constexpr bool
IsValidQmqpResponse(std::string_view payload) noexcept
{
	return !payload.empty() &&
		(payload.front() == 'K' || payload.front() == 'D' ||
		 payload.front() == 'Z');
}

void
BasicRelay::OnNetstringResponse(AllocatedArray<std::byte> &&payload) noexcept
{
	if (!IsValidQmqpResponse(ToStringView(payload))) {
		handler.OnRelayError("Zmalformed relay response"sv,
				     std::make_exception_ptr(std::runtime_error("Malformed QMQP response")));
		return;
	}

	handler.OnRelayResponse(ToStringView(payload));
}

void
BasicRelay::OnNetstringError(std::exception_ptr error) noexcept
{
	handler.OnRelayError("Zrelay failed"sv,
			     std::move(error));
}
