/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "StaticSocketAddress.hxx"

#include <assert.h>
#include <string.h>
#include <sys/un.h>

void
StaticSocketAddress::SetLocal(const char *path)
{
    auto &sun = reinterpret_cast<struct sockaddr_un &>(address);

    const size_t path_length = strlen(path);

    assert(path_length < sizeof(sun.sun_path));

    sun.sun_family = AF_LOCAL;
    memcpy(sun.sun_path, path, path_length + 1);

    if (sun.sun_path[0] == '@')
        /* abstract socket address */
        sun.sun_path[0] = 0;

    size = SUN_LEN(&sun);
}
