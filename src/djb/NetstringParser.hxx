// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#ifndef NETSTRING_PARSER_HXX
#define NETSTRING_PARSER_HXX

#include "util/Range.hxx"

#include <string_view>

class Error;

std::string_view
ParseNetstring(Range<const char *> &input);

#endif
