// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#include "NetstringParser.hxx"
#include "util/NumberParser.hxx"

#include <algorithm>

std::string_view
ParseNetstring(Range<const char *> &input_r)
{
	const Range<const char *> input = input_r;

	const char *colon = std::find(input.begin(), input.end(), ':');
	if (colon == input.end())
		return {};

	std::size_t value_size;
	if (!ParseIntegerTo({input.begin(), colon}, value_size))
		return {};

	const char *value = colon + 1;

	const size_t header_size = value - input.begin();
	size_t remaining = input.size() - header_size;
	if (value_size >= remaining || value[value_size] != ',')
		return {};

	input_r = { value + value_size + 1, input.end() };
	return { value, value_size };
}
