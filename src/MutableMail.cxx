// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "MutableMail.hxx"

void
MutableMail::InsertHeader(const char *name, const char *value)
{
	std::string line(name);
	line += ": ";
	line += value;
	line += "\r\n";

	headers.emplace_front(std::move(line));
}
