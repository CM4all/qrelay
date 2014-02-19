/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef MAIL_HXX
#define MAIL_HXX

#include "util/ConstBuffer.hxx"

#include <vector>

/**
 * An email received via QMQP.
 */
struct QmqpMail {
    /**
     * The message.
     */
    ConstBuffer<char> message;

    /**
     * The QMQP tail after the message, including the message's
     * trailing comma and the netstrings describing sender and all
     * recipients.  This is used to forward the message efficiently
     * after the message has been edited.
     */
    ConstBuffer<char> tail;

    /**
     * The sender email address.
     */
    ConstBuffer<char> sender;

    /**
     * The list of recipient email addresses.
     */
    std::vector<ConstBuffer<char>> recipients;

    bool Parse(ConstBuffer<char> input);
};

#endif
