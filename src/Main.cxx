/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "Config.hxx"
#include "CommandLine.hxx"
#include "event/Loop.hxx"
#include "event/SignalEvent.hxx"
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
    EventLoop &event_loop;

public:
    explicit QuitHandler(EventLoop &_event_loop):event_loop(_event_loop) {}

    void operator()() {
        cerr << "quit" << endl;
        event_loop.Break();
    }
};

static int
Run(const Config &config)
{
    EventLoop event_loop;

    signal(SIGPIPE, SIG_IGN);

    QuitHandler quit_handler(event_loop);
    const SignalEvent sigterm_event(SIGTERM, quit_handler),
        sigint_event(SIGINT, quit_handler),
        sigquit_event(SIGQUIT, quit_handler);

    Instance instance(config);

    Error error;
    if (!instance.qmqp_relay_server.ListenPath(config.listen.c_str(), error)) {
        instance.logger(error.GetMessage());
        return EXIT_FAILURE;
    }

    if (daemonize() < 0)
        return EXIT_FAILURE;

    event_loop.Dispatch();

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
