/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "TextFile.hxx"
#include "util/StringUtil.hxx"
#include "util/Error.hxx"

#include <string.h>

TextFile *
TextFile::Open(const char *path, Error &error)
{
    FILE *file = fopen(path, "rt");
    if (file == nullptr) {
        error.FormatErrno("Failed to open %s", path);
        return nullptr;
    }

    return new TextFile(path, file);
}

char *
TextFile::ReadLine()
{
    char *line = fgets(buffer, sizeof(buffer), file);
    if (line == nullptr)
        return nullptr;

    line = Strip(line);

    ++no;
    return line;
}

void
TextFile::PrefixError(Error &error)
{
    error.FormatPrefix("%s line %u: ", path, no);
}
