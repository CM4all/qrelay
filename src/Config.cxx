/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "Config.hxx"
#include "util/Tokenizer.hxx"
#include "util/Error.hxx"
#include "util/Domain.hxx"
#include "io/TextFile.hxx"

#include <string.h>

static constexpr Domain config_domain("config");

bool
Config::ParseLine(Tokenizer &tokenizer, Error &error)
{
    const char *key = tokenizer.NextWord(error);
    if (key == nullptr) {
        assert(error.IsDefined());
        return false;
    }

    if (strcmp(key, "listen") == 0) {
        const char *value = tokenizer.NextString(error);
        if (value == nullptr || *value == 0) {
            if (!error.IsDefined())
                error.Set(config_domain, "missing value");
            return false;
        }

        if (!listen.empty()) {
            error.Format(config_domain, "duplicate '%s'", key);
            return false;
        }

        listen = value;
        return true;
    } else if (strcmp(key, "connect") == 0) {
        const char *value = tokenizer.NextString(error);
        if (value == nullptr || *value == 0) {
            if (!error.IsDefined())
                error.Set(config_domain, "missing value");
            return false;
        }

        if (connect.IsDefined()) {
            error.Format(config_domain, "duplicate '%s'", key);
            return false;
        }

        return connect.Lookup(value, "qmqp", SOCK_STREAM, error);
    } else {
        error.Format(config_domain, "unknown option '%s'", key);
        return false;
    }
}

bool
Config::LoadFile(TextFile &file, Error &error)
{
    char *line;
    while ((line = file.ReadLine()) != nullptr) {
        Tokenizer tokenizer(line);
        if (tokenizer.IsEnd() || tokenizer.CurrentChar() == '#')
            continue;

        if (!ParseLine(tokenizer, error)) {
            file.PrefixError(error);
            return false;
        }

        if (!tokenizer.IsEnd()) {
            error.Set(config_domain, "too many arguments");
            file.PrefixError(error);
            return false;
        }
    }

    if (listen.empty()) {
        error.Format(config_domain, "no 'listen' in %s", file.GetPath());
        return false;
    }

    if (!connect.IsDefined()) {
        error.Format(config_domain, "no 'connect' in %s", file.GetPath());
        return false;
    }

    return true;
}

bool
Config::LoadFile(const char *path, Error &error)
{
    TextFile *file = TextFile::Open(path, error);
    if (file == nullptr)
        return false;

    const auto result = LoadFile(*file, error);
    delete file;
    return result;
}
