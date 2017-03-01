/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QRELAY_MUTABLE_MAIL_HXX
#define QRELAY_MUTABLE_MAIL_HXX

#include "djb/QmqpMail.hxx"
#include "util/AllocatedArray.hxx"

#include <forward_list>
#include <string>

#include <stdint.h>

/**
 * An extension of #QmqpMail which allows editing certain aspects of
 * the mail.
 */
struct MutableMail : QmqpMail {
    /**
     * The buffer where the #QmqpMail's #StringView instances point
     * into.
     */
    AllocatedArray<uint8_t> buffer;

    /**
     * A list of additional header lines (each ending with "\r\n")
     * which get inserted at the top of the mail.
     */
    std::forward_list<std::string> headers;

    explicit MutableMail(AllocatedArray<uint8_t> &&_buffer)
        :buffer(_buffer) {}

    bool Parse() {
        return QmqpMail::Parse({&buffer.front(), buffer.size()});
    }

    void InsertHeader(const char *name, const char *value);
};

#endif
