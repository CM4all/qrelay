/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QRELAY_CONFIG_HXX
#define QRELAY_CONFIG_HXX

#include "net/AllocatedSocketAddress.hxx"

#include <string>
#include <list>

class Error;
class Tokenizer;
class TextFile;
struct QmqpMail;

struct Config {
    struct Action {
        static constexpr unsigned MAX_EXEC = 32;

        enum class Type {
            UNDEFINED,

            /**
             * Discard the email, pretending it was accepted.
             */
            DISCARD,

            /**
             * Reject the email with a permanent error.
             */
            REJECT,

            /**
             * Connect to another QMQP server and relay to it.
             */
            CONNECT,

            /**
             * Execute a program that talks QMQP on stdin/stdout.
             */
            EXEC,
        };

        Type type;

        AllocatedSocketAddress connect;

        std::list<std::string> exec;

        Action():type(Type::UNDEFINED) {}

        bool IsDefined() const {
            return type != Type::UNDEFINED;
        }

        void ParseConnect(Tokenizer &tokenizer);
        void Parse(Tokenizer &tokenizer);
    };

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

    std::string listen;

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
