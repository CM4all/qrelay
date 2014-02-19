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
    struct Action {
        SocketAddress connect;

        Action() {
            connect.Clear();
        }

        bool IsDefined() const {
            return connect.IsDefined();
        }

        bool ParseConnect(Tokenizer &tokenizer, Error &error);
    };

    std::string listen;

    Action action;

    const Action &GetAction() const {
        return action;
    }

    bool ParseLine(Tokenizer &tokenizer, Error &error);
    bool LoadFile(TextFile &file, Error &error);
    bool LoadFile(const char *path, Error &error);
};

#endif
