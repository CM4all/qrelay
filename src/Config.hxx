/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QRELAY_CONFIG_HXX
#define QRELAY_CONFIG_HXX

#include "net/SocketAddress.hxx"

#include <string>

class Error;
class Tokenizer;
class TextFile;

struct Config {
    std::string listen;

    SocketAddress connect;

    Config() {
        connect.Clear();
    }

    bool ParseLine(Tokenizer &tokenizer, Error &error);
    bool LoadFile(TextFile &file, Error &error);
    bool LoadFile(const char *path, Error &error);
};

#endif
