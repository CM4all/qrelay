/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef NETSTRING_PARSER_HXX
#define NETSTRING_PARSER_HXX

#include "util/StringView.hxx"
#include "util/Range.hxx"

class Error;

StringView
ParseNetstring(Range<const char *> &input);

#endif
