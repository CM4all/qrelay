/*
 * author: Max Kellermann <mk@cm4all.com>
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

        if (columns[4].Equals(mountpoint))
            return {{columns[3].data, columns[3].size},
                    {columns[8].data, columns[8].size},
                    {columns[9].data, columns[9].size}};
    }

    /* not found: return empty string */
    return {{}, {}, {}};
}