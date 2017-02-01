/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "TextFile.hxx"
#include "system/Error.hxx"
#include "util/StringUtil.hxx"

TextFile::TextFile(const char *_path)
    :path(_path), file(fopen(_path, "rt")), no(0)
{
    if (file == nullptr)
        throw FormatErrno("Failed to open %s", path);
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
