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

#include "QmqpMail.hxx"
#include "NetstringParser.hxx"

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
	if (_sender.data() == nullptr)
		return false;

	sender = _sender;

	tail = MakeStringView(input.begin() - 1, input.end());

	do {
		auto value = ParseNetstring(input);
		if (value.data() == nullptr)
			return false;

		recipients.push_back(value);
	} while (!input.empty());

	return true;
}
