// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include <exception>
#include <string_view>

class RelayHandler {
public:
	virtual void OnRelayResponse(std::string_view response) noexcept = 0;
	virtual void OnRelayError(std::string_view response,
				  std::exception_ptr error) noexcept = 0;
};
