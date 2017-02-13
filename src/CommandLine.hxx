/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QRELAY_COMMAND_LINE_HXX
#define QRELAY_COMMAND_LINE_HXX

#include <string>

struct CommandLine {
    std::string config_path;
};

CommandLine
ParseCommandLine(int argc, char **argv);

#endif
