// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include <string_view>

#include <sysexits.h>

static constexpr std::string_view
ExitStatusToQmqpResponse(int status) noexcept
{
	using std::string_view_literals::operator""sv;

	switch (status) {
	case EX_USAGE:
	case EX_OSFILE:
	case EX_NOPERM:
	case EX_CONFIG:
		return "Dinternal server error"sv;

	default:
		return "Zinternal server error"sv;
	}
}
