// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#ifndef QRELAY_MUTABLE_MAIL_HXX
#define QRELAY_MUTABLE_MAIL_HXX

#include "djb/QmqpMail.hxx"
#include "util/AllocatedArray.hxx"

#include <forward_list>
#include <string>

#include <stdint.h>

/**
 * An extension of #QmqpMail which allows editing certain aspects of
 * the mail.
 */
struct MutableMail : QmqpMail {
	/**
	 * The buffer where the #QmqpMail's #std::string_view
	 * instances point into.
	 */
	AllocatedArray<std::byte> buffer;

	/**
	 * If the sender was modified, then this object owns the
	 * memory pointed to by QmqpMail::sender.
	 */
	std::string sender_buffer;

	/**
	 * A list of additional header lines (each ending with "\r\n")
	 * which get inserted at the top of the mail.
	 */
	std::forward_list<std::string> headers;

	explicit MutableMail(AllocatedArray<std::byte> &&_buffer)
		:buffer(_buffer) {}

	bool Parse() {
		return QmqpMail::Parse({(const std::byte *)buffer.data(), buffer.size()});
	}

	template<typename T>
	void SetSender(T &&new_sender) noexcept {
		sender = sender_buffer = std::forward<T>(new_sender);
	}

	void InsertHeader(const char *name, const char *value);
};

#endif
