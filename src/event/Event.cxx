/*
 * C++ wrappers for libevent.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "Event.hxx"

#include <inline/compiler.h>

void
Event::Callback(int fd, short mask, void *ctx)
{
    Event &event = *(Event *)ctx;

    event.handler(fd, mask);
}
