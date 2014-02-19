/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QRELAY_CONFIG_HXX
#define QRELAY_CONFIG_HXX

#include "net/SocketAddress.hxx"

#include <string>
#include <list>

class Error;
class Tokenizer;
class TextFile;
struct QmqpMail;

enum class TriBool {
    FALSE, TRUE, ERROR,
};

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
        bool Parse(Tokenizer &tokenizer, Error &error);
    };

    struct Condition {
        std::string recipient;

        bool IsDefined() const {
            return !recipient.empty();
        }

        TriBool Match(const QmqpMail &mail, Error &error) const;

        bool Parse(Tokenizer &tokenizer, Error &error);
    };

    struct Rule {
        Condition condition;
        Action action;

        bool Parse(Tokenizer &tokenizer, Error &error);
    };

    std::string listen;

    std::list<Rule> rules;

    Action action;

    const Action *GetAction(const QmqpMail &mail, Error &error) const;

    bool ParseLine(Tokenizer &tokenizer, Error &error);
    bool LoadFile(TextFile &file, Error &error);
    bool LoadFile(const char *path, Error &error);
};

#endif
