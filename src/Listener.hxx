// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#pragma once

#include "io/Logger.hxx"
#include "Connection.hxx"
#include "event/net/TemplateServerSocket.hxx"
#include "lua/ValuePtr.hxx"

using QmqpRelayListener =
	TemplateServerSocket<QmqpRelayConnection,
			     Instance &, std::size_t, Lua::ValuePtr,
			     RootLogger>;
