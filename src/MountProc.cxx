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

#include "MountProc.hxx"
#include "system/Error.hxx"
#include "util/ScopeExit.hxx"
#include "util/IterableSplitString.hxx"

#include <array>

#include <stdio.h>

template<std::size_t N>
static void
SplitFill(std::array<StringView, N> &dest, StringView s, char separator)
{
	std::size_t i = 0;
	for (auto value : IterableSplitString(s, separator)) {
		dest[i++] = value;
		if (i >= dest.size())
			return;
	}

	std::fill(std::next(dest.begin(), i), dest.end(), nullptr);
}

MountInfo
ReadProcessMount(unsigned pid, const char *_mountpoint)
{
	char path[64];
	sprintf(path, "/proc/%u/mountinfo", pid);
	FILE *file = fopen(path, "r");
	if (file == nullptr)
		throw FormatErrno("Failed to open %s", path);

	AtScopeExit(file) { fclose(file); };

	const StringView mountpoint(_mountpoint);

	char line[4096];
	while (fgets(line, sizeof(line), file) != nullptr) {
		std::array<StringView, 10> columns;
		SplitFill(columns, line, ' ');

		if (columns[4].Equals(mountpoint)) {
			/* skip the optional tagged fields */
			size_t i = 6;
			while (i < columns.size() && !columns[i].Equals("-"))
				++i;

			if (i + 2 < columns.size())
				return {{columns[3].data, columns[3].size},
						{columns[i + 1].data, columns[i + 1].size},
							{columns[i + 2].data, columns[i + 2].size}};
		}
	}

	/* not found: return empty string */
	return {{}, {}, {}};
}
