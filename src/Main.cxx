// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "CommandLine.hxx"
#include "Instance.hxx"
#include "LResolver.hxx"
#include "lib/fmt/RuntimeError.hxx"
#include "system/SetupProcess.hxx"
#include "net/AllocatedSocketAddress.hxx"
#include "lua/Value.hxx"
#include "lua/Util.hxx"
#include "lua/Error.hxx"
#include "lua/PushCClosure.hxx"
#include "lua/RunFile.hxx"
#include "lua/io/XattrTable.hxx"
#include "lua/net/SocketAddress.hxx"
#include "lua/event/Init.hxx"
#include "lua/pg/Init.hxx"
#include "util/PrintException.hxx"
#include "util/ScopeExit.hxx"

extern "C" {
#include <lauxlib.h>
#include <lualib.h>
}

#include <systemd/sd-daemon.h>

#include <stdio.h>
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
		throw FmtRuntimeError("`{}` must be a number", name);

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

	Lua::InitEvent(L, instance.GetEventLoop());
	Lua::InitPg(L, instance.GetEventLoop());

	Lua::InitSocketAddress(L);
	RegisterLuaResolver(L);

	static constexpr lua_Integer DEFAULT_MAX_SIZE = 16 * 1024 * 1024;
	Lua::SetGlobal(L, "max_size", DEFAULT_MAX_SIZE);

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

	Lua::InitXattrTable(L);

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

	instance.GetEventLoop().Run();

	return EXIT_SUCCESS;
}

int
main(int argc, char **argv) noexcept
try {
	const auto cmdline = ParseCommandLine(argc, argv);

	SetupProcess();

	/* force line buffering so Lua "print" statements are flushed
	   even if stdout is a pipe to systemd-journald */
	setvbuf(stdout, nullptr, _IOLBF, 0);
	setvbuf(stderr, nullptr, _IOLBF, 0);

	return Run(cmdline);
} catch (...) {
	PrintException(std::current_exception());
	return EXIT_FAILURE;
}
