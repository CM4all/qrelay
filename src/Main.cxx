/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "Config.hxx"
#include "CommandLine.hxx"
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

    SignalEvent sigterm_event, sigint_event, sigquit_event;

public:
    explicit QuitHandler(EventLoop &_event_loop)
        :event_loop(_event_loop),
         sigterm_event(event_loop, SIGTERM, BIND_THIS_METHOD(OnSignal)),
         sigint_event(event_loop, SIGINT, BIND_THIS_METHOD(OnSignal)),
         sigquit_event(event_loop, SIGQUIT, BIND_THIS_METHOD(OnSignal)) {
        sigterm_event.Add();
        sigint_event.Add();
        sigquit_event.Add();
    }

private:
    void OnSignal(int) {
        cerr << "quit" << endl;
        event_loop.Break();
    }
};

static int
Run(const Config &config)
{
    signal(SIGPIPE, SIG_IGN);

    Instance instance(config);

    QuitHandler quit_handler(instance.GetEventLoop());

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
