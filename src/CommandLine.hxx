/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef QRELAY_COMMAND_LINE_HXX
#define QRELAY_COMMAND_LINE_HXX

struct Config;

Config
ParseCommandLine(int argc, char **argv);

#endif
