/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "CgroupProc.hxx"
#include "system/Error.hxx"
#include "util/ScopeExit.hxx"
#include "util/IterableSplitString.hxx"

#include <stdio.h>

static bool
ListContains(StringView haystack, bool separator, StringView needle)
{
    for (auto value : IterableSplitString(haystack, separator))
        if (value.Equals(needle))
            return true;

    return false;
}

std::string
ReadProcessCgroup(unsigned pid, const char *_controller)
{
    char path[64];
    sprintf(path, "/proc/%u/cgroup", pid);
    FILE *file = fopen(path, "r");
    if (file == nullptr)
        throw FormatErrno("Failed to open %s", path);

    AtScopeExit(file) { fclose(file); };

    const StringView controller(_controller);

    char line[4096];
    while (fgets(line, sizeof(line), file) != nullptr) {
        char *colon = strchr(line, ':');
        if (colon == nullptr)
            continue;

        char *s = colon + 1;
        colon = strchr(s, ':');
        if (colon == nullptr)
            continue;

        if (ListContains(StringView(s, colon), ',', controller))
            return colon + 1;
    }

    /* not found: return empty string */
    return std::string();
}
