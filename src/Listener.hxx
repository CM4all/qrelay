// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#ifndef QMQP_RELAY_SERVER_HXX
#define QMQP_RELAY_SERVER_HXX

#include "io/Logger.hxx"
#include "Connection.hxx"
#include "event/net/TemplateServerSocket.hxx"
#include "lua/ValuePtr.hxx"

typedef TemplateServerSocket<QmqpRelayConnection,
			     size_t, Lua::ValuePtr,
			     RootLogger, EventLoop &> QmqpRelayListener;

#endif
