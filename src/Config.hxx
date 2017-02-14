/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QRELAY_CONFIG_HXX
#define QRELAY_CONFIG_HXX

#include "Action.hxx"
#include "net/AllocatedSocketAddress.hxx"

#include <string>
#include <list>

class Error;
class Tokenizer;
class TextFile;
struct QmqpMail;

struct Config {
    struct Condition {
        std::string recipient;

        bool IsDefined() const {
            return !recipient.empty();
        }

        /**
         * Throws std::runtime_error on error.
         */
        bool Match(const QmqpMail &mail) const;

        void Parse(Tokenizer &tokenizer);
    };

    struct Rule {
        Condition condition;
        Action action;

        void Parse(Tokenizer &tokenizer);
    };

    AllocatedSocketAddress listen;

    std::list<Rule> rules;

    Action action;

    /**
     * Throws std::runtime_error on error.
     */
    const Action *GetAction(const QmqpMail &mail) const;

    void ParseLine(Tokenizer &tokenizer);
    void LoadFile(TextFile &file);
    void LoadFile(const char *path);
};

#endif
