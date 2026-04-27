// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#include "djb/NetstringParser.hxx"

#include <gtest/gtest.h>

#include <array>
#include <string_view>

using std::string_view_literals::operator""sv;

namespace {

Range<const char *>
MakeRange(std::string_view s) noexcept
{
	return {s.data(), s.data() + s.size()};
}

std::string_view
Remaining(Range<const char *> r) noexcept
{
	return {r.begin(), static_cast<std::size_t>(r.end() - r.begin())};
}

} // namespace

TEST(NetstringParser, Basic)
{
	auto input = MakeRange("3:foo,"sv);
	const auto value = ParseNetstring(input);

	EXPECT_EQ(value, "foo"sv);
	EXPECT_EQ(Remaining(input), ""sv);
}

TEST(NetstringParser, Empty)
{
	auto input = MakeRange("0:,"sv);
	const auto value = ParseNetstring(input);

	EXPECT_NE(value.data(), nullptr);
	EXPECT_EQ(value, ""sv);
	EXPECT_EQ(Remaining(input), ""sv);
}

TEST(NetstringParser, AdvancesToNextValue)
{
	auto input = MakeRange("3:foo,4:tail,"sv);

	EXPECT_EQ(ParseNetstring(input), "foo"sv);
	EXPECT_EQ(Remaining(input), "4:tail,"sv);

	EXPECT_EQ(ParseNetstring(input), "tail"sv);
	EXPECT_EQ(Remaining(input), ""sv);
}

TEST(NetstringParser, MalformedDoesNotAdvance)
{
	static constexpr std::array tests{
		""sv,
		":"sv,
		":foo,"sv,
		"foo"sv,
		"foo:bar,"sv,
		" 3:foo,"sv,
		"+3:foo,"sv,
		"-3:foo,"sv,
		"1.0:x,"sv,
		"1e0:x,"sv,
		"18446744073709551616:x,"sv,
		"3:foo"sv,
		"3:fo,"sv,
		"3:foo;"sv,
	};

	for (const auto test : tests) {
		auto input = MakeRange(test);
		const auto old_begin = input.begin();
		const auto old_end = input.end();

		const auto value = ParseNetstring(input);

		EXPECT_EQ(value.data(), nullptr) << test;
		EXPECT_EQ(input.begin(), old_begin) << test;
		EXPECT_EQ(input.end(), old_end) << test;
	}
}
