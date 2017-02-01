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

public:
    explicit TextFile(const char *_path);

    ~TextFile() {
        fclose(file);
    }

    const char *GetPath() const {
        return path;
    }

    unsigned GetLineNumber() const {
        return no;
    }

    char *ReadLine();

    void PrefixError(Error &error);
};

#endif
