// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#include "NetstringParser.hxx"
#include "util/NumberParser.hxx"
#include "util/StringSplit.hxx"

std::string_view
ParseNetstring(std::string_view &input_r) noexcept
{
	const std::string_view input = input_r;

	const auto [value_size_string, rest] = Split(input, ':');
	if (rest.data() == nullptr)
		return {};

	std::size_t value_size;
	if (!ParseIntegerTo(value_size_string, value_size))
		return {};

	if (value_size > 0 && value_size_string.front() == '0')
		/* reject leading zeroes */
		return {};

	if (value_size >= rest.size() || rest[value_size] != ',')
		return {};

	input_r = rest.substr(value_size + 1);
	return rest.substr(0, value_size);
}
