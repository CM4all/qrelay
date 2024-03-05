// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include "net/AllocatedSocketAddress.hxx"
#include "util/StaticVector.hxx"

#include <string>

struct Action {
	static constexpr unsigned MAX_EXEC = 32;
	static constexpr unsigned MAX_ENV = 32;

	enum class Type {
		UNDEFINED,

		/**
		 * Discard the email, pretending it was accepted.
		 */
		DISCARD,

		/**
		 * Reject the email with a permanent error.
		 */
		REJECT,

		/**
		 * Connect to another QMQP server and relay to it.
		 */
		CONNECT,

		/**
		 * Execute a program that talks QMQP on stdin/stdout.
		 */
		EXEC,

		/**
		 * Execute a program that reads the email message from
		 * stdin.  The envelope is ignored.
		 */
		EXEC_RAW,
	};

	Type type = Type::UNDEFINED;

	AllocatedSocketAddress connect;

	StaticVector<std::string, MAX_EXEC> exec;

	/**
	 * Environment variables for #EXEC and #EXEC_RAW.
	 */
	StaticVector<std::string, MAX_ENV> env;

	bool IsDefined() const {
		return type != Type::UNDEFINED;
	}
};
