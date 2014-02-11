/*
 * OO representation of a struct sockaddr.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "SocketAddress.hxx"
#include "Error.hxx"
#include "util/Error.hxx"

#include <assert.h>
#include <string.h>
#include <sys/un.h>
#include <netdb.h>

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

bool
SocketAddress::Lookup(const char *host, const char *service, int socktype,
                      Error &error)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = socktype;

    struct addrinfo *ai;
    int result = getaddrinfo(host, service, &hints, &ai);
    if (result != 0) {
        error.Format(netdb_domain, "Failed to look up '%s': %s",
                     host, gai_strerror(result));
        return false;
    }

    size = ai->ai_addrlen;
    assert(size <= sizeof(address));

    memcpy(reinterpret_cast<void *>(&address),
           reinterpret_cast<void *>(ai->ai_addr), size);
    freeaddrinfo(ai);
    return true;
}
