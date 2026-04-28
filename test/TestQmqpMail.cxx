// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#include "djb/QmqpMail.hxx"
#include "util/SpanCast.hxx"

#include <gtest/gtest.h>

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

using std::string_view_literals::operator""sv;

namespace {

std::string
Netstring(std::string_view payload)
{
	std::string result = std::to_string(payload.size());
	result.push_back(':');
	result.append(payload);
	result.push_back(',');
	return result;
}

std::string
MakeQmqp(std::string_view message, std::string_view sender,
	 std::initializer_list<std::string_view> recipients)
{
	std::string payload = Netstring(message);
	payload += Netstring(sender);

	for (const auto recipient : recipients)
		payload += Netstring(recipient);

	return payload;
}

} // namespace

TEST(QmqpMail, Basic)
{
	const auto payload = MakeQmqp("Subject: Hello!\r\n\r\nBody\r\n"sv,
				      "sender@example.com"sv,
				      {"one@example.com"sv, "two@example.com"sv});

	QmqpMail mail;
	ASSERT_TRUE(mail.Parse(AsBytes(payload)));

	EXPECT_EQ(mail.message, "Subject: Hello!\r\n\r\nBody\r\n"sv);
	EXPECT_EQ(mail.sender, "sender@example.com"sv);
	ASSERT_EQ(mail.recipients.size(), 2U);
	EXPECT_EQ(mail.recipients[0], "one@example.com"sv);
	EXPECT_EQ(mail.recipients[1], "two@example.com"sv);

	const auto expected_tail =
		"," + Netstring("one@example.com"sv) + Netstring("two@example.com"sv);
	EXPECT_EQ(mail.tail, expected_tail);
}

TEST(QmqpMail, EmptyMessage)
{
	const auto payload = MakeQmqp(""sv, "sender@example.com"sv,
				      {"recipient@example.com"sv});

	QmqpMail mail;
	ASSERT_TRUE(mail.Parse(AsBytes(payload)));

	EXPECT_NE(mail.message.data(), nullptr);
	EXPECT_EQ(mail.message, ""sv);
	EXPECT_EQ(mail.sender, "sender@example.com"sv);
	ASSERT_EQ(mail.recipients.size(), 1U);
	EXPECT_EQ(mail.recipients[0], "recipient@example.com"sv);
}

TEST(QmqpMail, RejectsMalformedEnvelope)
{
	static const std::vector<std::string> tests{
		MakeQmqp("message"sv, "not an address"sv, {"recipient@example.com"sv}),
		MakeQmqp("message"sv, "sender@example.com"sv, {"not an address"sv}),
		MakeQmqp("message"sv, "sender@example.com"sv, {}),
		Netstring("message"sv) + " 18:sender@example.com," +
			Netstring("recipient@example.com"sv),
		Netstring("message"sv) + Netstring("sender@example.com"sv) +
			" 21:recipient@example.com,",
		Netstring("message"sv) + Netstring("sender@example.com"sv) +
			Netstring("recipient@example.com"sv) + "garbage",
	};

	for (const auto &payload : tests) {
		QmqpMail mail;
		EXPECT_FALSE(mail.Parse(AsBytes(payload))) << payload;
	}
}
