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

static int
Run(const CommandLine &cmdline)
{
    Config config;
    config.LoadFile(cmdline.config_path.c_str());

    Instance instance;
    instance.AddQmqpRelayServer(config);

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

    SetupProcess();

    const int result = Run(cmdline);

    daemonize_cleanup();

    cerr << "exiting" << endl;
    return result;
} catch (const std::exception &e) {
    cerr << "error: " << e << endl;
    return EXIT_FAILURE;
}
