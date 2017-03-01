/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QRELAY_MUTABLE_MAIL_HXX
#define QRELAY_MUTABLE_MAIL_HXX

#include "djb/QmqpMail.hxx"

/**
 * An extension of #QmqpMail which allows editing certain aspects of
 * the mail.
 */
struct MutableMail : QmqpMail {
};

#endif
