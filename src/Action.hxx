/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QRELAY_ACTION_HXX
#define QRELAY_ACTION_HXX

#include "net/AllocatedSocketAddress.hxx"

#include <string>
#include <list>

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

    Type type = Type::UNDEFINED;

    AllocatedSocketAddress connect;

    std::list<std::string> exec;

    bool IsDefined() const {
        return type != Type::UNDEFINED;
    }
};

#endif
