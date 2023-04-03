// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#ifndef CGROUP_PROC_HXX
#define CGROUP_PROC_HXX

#include <string>

/**
 * Determine the cgroup path the specified process is member of.
 *
 * Throws std::runtime_error on error.
 *
 * @return the path within the specified controller (starting with a
 * slash), or an empty string if the controller was not found in
 * /proc/PID/cgroup
 */
std::string
ReadProcessCgroup(unsigned pid, const char *controller);

#endif
