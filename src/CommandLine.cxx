/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "CommandLine.hxx"
#include "Config.hxx"
#include "util/Error.hxx"

#include <daemon/daemonize.h>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <string>
#include <vector>
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include <cstdlib>

Config
ParseCommandLine(int argc, char **argv)
{
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("listen", po::value<std::string>()->required(),
         "listen on this UNIX domain socket")
        ("connect", po::value<std::string>()->required(),
         "connect to this QMQP server")
        ("no-daemon", "don't daemonize")
        ("pidfile", po::value<std::string>(), "create a pid file")
        ("user", po::value<std::string>(), "switch to another user id")
        ("logger-user", po::value<std::string>(),
         "execute the logger program with this user id")
        ("logger", po::value<std::string>(),
         "specifies a logger program (executed by /bin/sh)")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    if (vm.count("help")) {
        cout << desc << endl;
        exit(EXIT_SUCCESS);
    }

    po::notify(vm);

    if (vm.count("no-daemon"))
        daemon_config.detach = false;

    if (vm.count("pidfile")) {
        static std::string buffer;
        buffer = vm["pidfile"].as<std::string>();
        daemon_config.pidfile = buffer.c_str();
    }

    if (vm.count("user")) {
        const std::string user = vm["user"].as<std::string>();
        if (daemon_user_by_name(&daemon_config.user, user.c_str(), NULL) < 0) {
            cerr << "Invalid user: " << user << endl;
            exit(EXIT_FAILURE);
        }
    }

    if (vm.count("logger")) {
        static std::string buffer;
        buffer = vm["logger"].as<std::string>();
        daemon_config.logger = buffer.c_str();
    }

    if (vm.count("logger-user")) {
        const std::string user = vm["logger-user"].as<std::string>();
        if (daemon_user_by_name(&daemon_config.logger_user,
                                user.c_str(), NULL) < 0) {
            cerr << "Invalid user: " << user << endl;
            exit(EXIT_FAILURE);
        }
    }

    Config config;
    config.listen = vm["listen"].as<std::string>();

    Error error;
    if (!config.connect.Lookup(vm["connect"].as<std::string>().c_str(),
                               "qmqp", SOCK_STREAM, error)) {
        cerr << error.GetMessage() << endl;
        exit(EXIT_FAILURE);
    }

    return config;
}
