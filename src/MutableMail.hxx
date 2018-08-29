/*
 * Copyright 2014-2018 Content Management AG
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
	 * The buffer where the #QmqpMail's #StringView instances point
	 * into.
	 */
	AllocatedArray<uint8_t> buffer;

	/**
	 * A list of additional header lines (each ending with "\r\n")
	 * which get inserted at the top of the mail.
	 */
	std::forward_list<std::string> headers;

	explicit MutableMail(AllocatedArray<uint8_t> &&_buffer)
		:buffer(_buffer) {}

	bool Parse() {
		return QmqpMail::Parse({&buffer.front(), buffer.size()});
	}

	void InsertHeader(const char *name, const char *value);
};

#endif
