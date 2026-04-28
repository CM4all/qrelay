// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#include "QmqpMail.hxx"
#include "NetstringParser.hxx"
#include "uri/EmailAddress.hxx"
#include "util/SpanCast.hxx"

static inline constexpr std::string_view
MakeStringView(const char *begin, const char *end)
{
	return {begin, size_t(end - begin)};
}

QmqpMail::ParseResult
QmqpMail::Parse(std::span<const std::byte> __input) noexcept
{
	std::string_view input = ToStringView(__input);

	auto _message = ParseNetstring(input);
	if (_message.data() == nullptr)
		return ParseResult::MALFORMED;

	message = _message;

	auto _sender = ParseNetstring(input);
	if (_sender.data() == nullptr || !VerifyEmailAddress(_sender))
		return ParseResult::MALFORMED;

	sender = _sender;

	tail = MakeStringView(input.begin() - 1, input.end());

	do {
		auto value = ParseNetstring(input);
		if (value.data() == nullptr || !VerifyEmailAddress(value))
			return ParseResult::MALFORMED;

		recipients.push_back(value);
	} while (!input.empty());

	return ParseResult::SUCCESS;
}
