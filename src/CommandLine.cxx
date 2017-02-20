/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "CommandLine.hxx"

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
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    if (vm.count("help")) {
        cout << desc << endl;
        exit(EXIT_SUCCESS);
    }

    po::notify(vm);

    return { vm["config"].as<std::string>() };
}
