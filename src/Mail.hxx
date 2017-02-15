/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef MAIL_HXX
#define MAIL_HXX

#include "util/StringView.hxx"

#include <vector>

/**
 * An email received via QMQP.
 */
struct QmqpMail {
    /**
     * The message.
     */
    StringView message;

    /**
     * The QMQP tail after the message, including the message's
     * trailing comma and the netstrings describing sender and all
     * recipients.  This is used to forward the message efficiently
     * after the message has been edited.
     */
    StringView tail;

    /**
     * The sender email address.
     */
    StringView sender;

    /**
     * The list of recipient email addresses.
     */
    std::vector<StringView> recipients;

    bool Parse(ConstBuffer<void> input);
};

#endif
