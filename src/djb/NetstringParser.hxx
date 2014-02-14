/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef NETSTRING_PARSER_HXX
#define NETSTRING_PARSER_HXX

#include "util/ConstBuffer.hxx"
#include "util/Range.hxx"

class Error;

ConstBuffer<char>
ParseNetstring(Range<const char *> &input);

#endif
