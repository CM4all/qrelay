// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#pragma once

#include <string>

struct CommandLine {
	std::string config_path = "/etc/cm4all/qrelay/config.lua";
};

CommandLine
ParseCommandLine(int argc, char **argv);
