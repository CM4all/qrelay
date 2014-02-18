/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef TEXT_FILE_HXX
#define TEXT_FILE_HXX

#include <stdio.h>

class Error;

/**
 * Read a file line-by-line.
 */
class TextFile {
    const char *const path;
    FILE *const file;

    unsigned no;

    char buffer[4096];

    TextFile(const char *_path, FILE *_file)
        :path(_path), file(_file), no(0) {}

public:
    ~TextFile() {
        fclose(file);
    }

    static TextFile *Open(const char *path, Error &error);

    const char *GetPath() const {
        return path;
    }

    char *ReadLine();

    void PrefixError(Error &error);
};

#endif
