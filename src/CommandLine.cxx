/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "CommandLine.hxx"

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

CommandLine
ParseCommandLine(int argc, char **argv)
{
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("config", po::value<std::string>()->required(),
         "load this configuration file")
        ("user", po::value<std::string>(), "switch to another user id")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    if (vm.count("help")) {
        cout << desc << endl;
        exit(EXIT_SUCCESS);
    }

    po::notify(vm);

    if (vm.count("user")) {
        const std::string user = vm["user"].as<std::string>();
        if (daemon_user_by_name(&daemon_config.user, user.c_str(), NULL) < 0) {
            cerr << "Invalid user: " << user << endl;
            exit(EXIT_FAILURE);
        }
    }

    return { vm["config"].as<std::string>() };
}
