/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QRELAY_CONFIG_HXX
#define QRELAY_CONFIG_HXX

#include "net/SocketAddress.hxx"

#include <string>

struct Config {
    std::string listen;

    SocketAddress connect;
};

#endif
