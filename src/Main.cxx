/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "Config.hxx"
#include "CommandLine.hxx"
#include "Instance.hxx"
#include "system/SetupProcess.hxx"
#include "util/OstreamException.hxx"

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
    Instance instance(config);

    instance.qmqp_relay_server.Listen(config.listen);

    if (daemonize() < 0)
        return EXIT_FAILURE;

    instance.GetEventLoop().Dispatch();

    return EXIT_SUCCESS;
}

int
main(int argc, char **argv)
try {
    daemon_config.detach = false;

    const auto cmdline = ParseCommandLine(argc, argv);

    Config config;
    config.LoadFile(cmdline.config_path.c_str());

    SetupProcess();

    const int result = Run(config);

    daemonize_cleanup();

    cerr << "exiting" << endl;
    return result;
} catch (const std::exception &e) {
    cerr << "error: " << e << endl;
    return EXIT_FAILURE;
}
