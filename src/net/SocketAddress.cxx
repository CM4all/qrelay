/*
 * OO representation of a struct sockaddr.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "SocketAddress.hxx"

#include <assert.h>
#include <sys/un.h>

void
SocketAddress::SetLocal(const char *path)
{
    auto &sun = reinterpret_cast<struct sockaddr_un &>(address);

    const size_t path_length = strlen(path);

    assert(path_length < sizeof(sun.sun_path));

    sun.sun_family = AF_LOCAL;
    memcpy(sun.sun_path, path, path_length + 1);

    size = SUN_LEN(&sun);
}
