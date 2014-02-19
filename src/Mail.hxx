/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef MAIL_HXX
#define MAIL_HXX

#include "util/ConstBuffer.hxx"

#include <vector>

struct QmqpMail {
    ConstBuffer<char> message;
    ConstBuffer<char> tail;

    ConstBuffer<char> sender;
    std::vector<ConstBuffer<char>> recipients;

    bool Parse(ConstBuffer<char> input);
};

#endif
