// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#include "MutableMail.hxx"

#include <fmt/core.h>

using std::string_view_literals::operator""sv;

void
MutableMail::InsertHeader(const char *name, const char *value)
{
	headers.emplace_front(fmt::format("{}: {}\r\n"sv, name, value));
}
