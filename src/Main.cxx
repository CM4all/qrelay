/*
 * Copyright 2014-2021 CM4all GmbH
 * All rights reserved.
 *
 * author: Max Kellermann <mk@cm4all.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "CommandLine.hxx"
#include "Instance.hxx"
#include "LAddress.hxx"
#include "LResolver.hxx"
#include "system/SetupProcess.hxx"
#include "net/AllocatedSocketAddress.hxx"
#include "lua/Value.hxx"
#include "lua/Util.hxx"
#include "lua/Error.hxx"
#include "lua/RunFile.hxx"
#include "util/PrintException.hxx"
#include "util/RuntimeError.hxx"
#include "util/ScopeExit.hxx"

extern "C" {
#include <lauxlib.h>
#include <lualib.h>
}

#include <systemd/sd-daemon.h>

#include <stdlib.h>

/**
 * A "magic" pointer used to identify our artificial "systemd" Lua
 * keyword, which is wrapped as "light user data".
 */
static int systemd_magic = 42;

static bool
IsSystemdMagic(lua_State *L, int idx)
{
	return lua_islightuserdata(L, idx) &&
		lua_touserdata(L, idx) == &systemd_magic;
}

static auto
GetGlobalInt(lua_State *L, const char *name)
{
	lua_getglobal(L, name);
	AtScopeExit(L) { lua_pop(L, 1); };

	if (!lua_isnumber(L, -1))
		throw FormatRuntimeError("`%s` must be a number", name);

	return lua_tointeger(L, -1);
}

static int
l_qmqp_listen(lua_State *L)
try {
	auto &instance = *(Instance *)lua_touserdata(L, lua_upvalueindex(1));

	if (lua_gettop(L) != 2)
		return luaL_error(L, "Invalid parameter count");

	if (!lua_isfunction(L, 2))
		return luaL_argerror(L, 2, "function expected");

	const auto max_size = GetGlobalInt(L, "max_size");
	if (max_size < 1024)
		throw std::runtime_error("`max_size` is too small");
	if (max_size > 1024 * 1024 * 1024)
		throw std::runtime_error("`max_size` is too large");

	auto handler = std::make_shared<Lua::Value>(L, Lua::StackIndex(2));

	if (IsSystemdMagic(L, 1)) {
		instance.AddSystemdListener(max_size, std::move(handler));
	} else if (lua_isstring(L, 1)) {
		const char *address_string = lua_tostring(L, 1);

		AllocatedSocketAddress address;
		address.SetLocal(address_string);

		instance.AddListener(address, max_size, std::move(handler));
	} else
		luaL_argerror(L, 1, "path expected");

	return 0;
} catch (...) {
	Lua::RaiseCurrent(L);
}

static void
SetupConfigState(lua_State *L, Instance &instance)
{
	luaL_openlibs(L);

	RegisterLuaAddress(L);
	RegisterLuaResolver(L);

	Lua::SetGlobal(L, "max_size", 16 * 1024 * 1024);

	Lua::SetGlobal(L, "systemd", Lua::LightUserData(&systemd_magic));

	Lua::SetGlobal(L, "qmqp_listen",
		       Lua::MakeCClosure(l_qmqp_listen,
					 Lua::LightUserData(&instance)));
}

static void
SetupRuntimeState(lua_State *L)
{
	Lua::SetGlobal(L, "max_size", nullptr);
	Lua::SetGlobal(L, "qmqp_listen", nullptr);

	QmqpRelayConnection::Register(L);

	UnregisterLuaResolver(L);
}

static int
Run(const CommandLine &cmdline)
{
	Instance instance;
	SetupConfigState(instance.GetLuaState(), instance);

	Lua::RunFile(instance.GetLuaState(), cmdline.config_path.c_str());

	instance.Check();

	SetupRuntimeState(instance.GetLuaState());

	/* tell systemd we're ready */
	sd_notify(0, "READY=1");

	instance.GetEventLoop().Dispatch();

	return EXIT_SUCCESS;
}

int
main(int, char **) noexcept
try {
	const CommandLine cmdline;

	SetupProcess();

	return Run(cmdline);
} catch (...) {
	PrintException(std::current_exception());
	return EXIT_FAILURE;
}
