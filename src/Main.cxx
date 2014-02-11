/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "Config.hxx"
#include "CommandLine.hxx"
#include "Event.hxx"
#include "Instance.hxx"
#include "util/Error.hxx"

#include <daemon/daemonize.h>

#include <iostream>
using std::cerr;
using std::endl;

#include <stdlib.h>
#include <signal.h>
#include <sys/prctl.h>

class QuitHandler {
    EventBase &base;

public:
    QuitHandler(EventBase &_base):base(_base) {}

    void operator()() {
        cerr << "quit" << endl;
        base.Break();
    }
};

static int
Run(const Config &config)
{
    EventBase event_base;

    QuitHandler quit_handler(event_base);
    const SignalEvent sigterm_event(SIGTERM, quit_handler),
        sigint_event(SIGINT, quit_handler),
        sigquit_event(SIGQUIT, quit_handler);

    Instance instance;

    Error error;
    if (!instance.qmqp_relay_server.ListenPath(config.listen.c_str(), error)) {
        instance.logger(error.GetMessage());
        return EXIT_FAILURE;
    }

    if (daemonize() < 0)
        return EXIT_FAILURE;

    event_base.Dispatch();

    return EXIT_SUCCESS;
}

int
main(int argc, char **argv)
{
    Config config;

    try {
        config = ParseCommandLine(argc, argv);
    } catch (const std::exception &e) {
        cerr << "error: " << e.what() << endl;
        return EXIT_FAILURE;
    }

    /* timer slack 500ms - we don't care for timer correctness */
    prctl(PR_SET_TIMERSLACK, 500000000, 0, 0, 0);

    const int result = Run(config);

    daemonize_cleanup();

    cerr << "exiting" << endl;
    return result;
}
