/*
 * C++ wrappers for libevent.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "FunctionEvent.hxx"

#include <inline/compiler.h>

void
FunctionEvent::Callback(int fd, short mask, void *ctx)
{
    FunctionEvent &event = *(FunctionEvent *)ctx;

    event.handler(fd, mask);
}
