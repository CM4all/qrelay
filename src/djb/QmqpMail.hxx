// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#pragma once

#include <span>
#include <string_view>
#include <vector>

/**
 * An email received via QMQP.
 */
struct QmqpMail {
	static constexpr std::size_t MAX_RECIPIENTS = 1024;

	/**
	 * The message.
	 */
	std::string_view message;

	/**
	 * The QMQP tail after the message and the sender, including
	 * the message's trailing comma and the netstrings describing
	 * all recipients.  This is used to forward the message
	 * efficiently after the message has been edited.
	 */
	std::string_view tail;

	/**
	 * The sender email address.
	 */
	std::string_view sender;

	/**
	 * The list of recipient email addresses.
	 */
	std::vector<std::string_view> recipients;

	enum class ParseResult {
		/**
		 * Parsing has succeeded and all fields contain valid
		 * values.
		 */
		SUCCESS,

		/**
		 * The input was malformed (bad Netstring or QMQP
		 * syntax).
		 */
		MALFORMED,

		/**
		 * At least one envelope address was bad.
		 */
		BAD_ADDRESS,

		/**
		 * Too many recipients.  This software has a
		 * hard-coded limit for that.
		 */
		TOO_MANY_RECIPIENTS,
	};

	ParseResult Parse(std::span<const std::byte> input) noexcept;
};
