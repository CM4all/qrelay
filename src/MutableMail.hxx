/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QRELAY_MUTABLE_MAIL_HXX
#define QRELAY_MUTABLE_MAIL_HXX

#include "djb/QmqpMail.hxx"
#include "util/AllocatedArray.hxx"

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

    explicit MutableMail(AllocatedArray<uint8_t> &&_buffer)
        :buffer(_buffer) {}

    bool Parse() {
        return QmqpMail::Parse({&buffer.front(), buffer.size()});
    }
};

#endif
