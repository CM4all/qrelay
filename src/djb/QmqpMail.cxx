// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "QmqpMail.hxx"
#include "NetstringParser.hxx"
#include "uri/EmailAddress.hxx"

static inline constexpr std::string_view
MakeStringView(const char *begin, const char *end)
{
	return {begin, size_t(end - begin)};
}

bool
QmqpMail::Parse(std::span<const std::byte> __input) noexcept
{
	const std::span<const char> _input{(const char *)__input.data(), __input.size()};
	Range<const char *> input(_input.data(), _input.data() + _input.size());

	auto _message = ParseNetstring(input);
	if (_message.data() == nullptr)
		return false;

	message = _message;

	auto _sender = ParseNetstring(input);
	if (_sender.data() == nullptr || !VerifyEmailAddress(_sender))
		return false;

	sender = _sender;

	tail = MakeStringView(input.begin() - 1, input.end());

	do {
		auto value = ParseNetstring(input);
		if (value.data() == nullptr || !VerifyEmailAddress(value))
			return false;

		recipients.push_back(value);
	} while (!input.empty());

	return true;
}
