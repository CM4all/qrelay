/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "Config.hxx"
#include "CommandLine.hxx"
#include "Instance.hxx"
#include "util/Error.hxx"

#include <daemon/daemonize.h>

#include <iostream>
using std::cerr;
using std::endl;

#include <stdlib.h>
#include <signal.h>
#include <sys/prctl.h>

static int
Run(const Config &config)
{
    signal(SIGPIPE, SIG_IGN);

    Instance instance(config);

    Error error;
    if (!instance.qmqp_relay_server.ListenPath(config.listen.c_str(), error)) {
        instance.logger(error.GetMessage());
        return EXIT_FAILURE;
    }

    if (daemonize() < 0)
        return EXIT_FAILURE;

    instance.GetEventLoop().Dispatch();

    return EXIT_SUCCESS;
}

int
main(int argc, char **argv)
try {
    daemon_config.detach = false;

    const auto config = ParseCommandLine(argc, argv);

    /* timer slack 500ms - we don't care for timer correctness */
    prctl(PR_SET_TIMERSLACK, 500000000, 0, 0, 0);

    const int result = Run(config);

    daemonize_cleanup();

    cerr << "exiting" << endl;
    return result;
} catch (const std::exception &e) {
    cerr << "error: " << e.what() << endl;
    return EXIT_FAILURE;
}
