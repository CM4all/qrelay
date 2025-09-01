// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#pragma once

/**
 * Set up the current process by applying some common settings.
 *
 * - ignore SIGPIPE
 * - increase timer slack
 * - disable pthread cancellation
 */
void
SetupProcess() noexcept;
