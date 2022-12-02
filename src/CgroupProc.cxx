/*
 * Copyright 2014-2021 CM4all GmbH
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

#include "CgroupProc.hxx"
#include "lib/fmt/ToBuffer.hxx"
#include "lib/fmt/SystemError.hxx"
#include "util/ScopeExit.hxx"
#include "util/IterableSplitString.hxx"
#include "util/StringSplit.hxx"

static bool
ListContains(std::string_view haystack, char separator, std::string_view needle)
{
	for (const auto value : IterableSplitString(haystack, separator))
		if (value == needle)
			return true;

	return false;
}

static std::string_view
StripTrailingNewline(std::string_view s)
{
	return SplitWhile(s, [](char ch){ return ch != '\n'; }).first;
}

std::string
ReadProcessCgroup(unsigned pid, const char *_controller)
{
	const auto path = FmtBuffer<64>("/proc/{}/cgroup", pid);
	FILE *file = fopen(path, "r");
	if (file == nullptr)
		throw FmtErrno("Failed to open {}", path);

	AtScopeExit(file) { fclose(file); };

	const std::string_view controller(_controller);

	char line[4096];
	while (fgets(line, sizeof(line), file) != nullptr) {
		const auto [controllers, group] = Split(Split(std::string_view{line}, ':').second, ':');
		if (!group.empty() && ListContains(controllers, ',', controller))
			return std::string{StripTrailingNewline(group)};
	}

	/* not found: return empty string */
	return std::string();
}
