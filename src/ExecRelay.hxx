// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include "BasicRelay.hxx"
#include "net/djb/NetstringGenerator.hxx"

struct Action;

class ExecRelay final
	: BasicRelay
{
public:
	using BasicRelay::BasicRelay;

	bool Start(const Action &action) noexcept;
};
