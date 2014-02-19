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

        void Clear() {
            connect.Clear();
        }

        bool IsDefined() const {
            return connect.IsDefined();
        }
    };

    std::string listen;

    Action action;

    Config() {
        action.Clear();
    }

    const Action &GetAction() const {
        return action;
    }

    bool ParseLine(Tokenizer &tokenizer, Error &error);
    bool LoadFile(TextFile &file, Error &error);
    bool LoadFile(const char *path, Error &error);
};

#endif
