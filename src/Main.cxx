// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "CommandLine.hxx"
#include "Instance.hxx"
#include "LResolver.hxx"
#include "lib/fmt/RuntimeError.hxx"
#include "lib/fmt/SystemError.hxx"
#include "system/SetupProcess.hxx"
#include "net/AllocatedSocketAddress.hxx"
#include "lua/Value.hxx"
#include "lua/Util.hxx"
#include "lua/Error.hxx"
#include "lua/PushCClosure.hxx"
#include "lua/RunFile.hxx"
#include "lua/io/XattrTable.hxx"
#include "lua/io/CgroupInfo.hxx"
#include "lua/net/SocketAddress.hxx"
#include "lua/event/Init.hxx"
#include "util/PrintException.hxx"
#include "util/ScopeExit.hxx"
#include "config.h"

#ifdef HAVE_PG
#include "lua/pg/Init.hxx"
#endif

#ifdef HAVE_LIBSODIUM
#include "lua/sodium/Init.hxx"
#endif

extern "C" {
#include <lauxlib.h>
#include <lualib.h>
}

#ifdef HAVE_LIBSYSTEMD
#include <systemd/sd-daemon.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // for chdir()

#ifdef HAVE_LIBSYSTEMD

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

#endif // HAVE_LIBSYSTEMD

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

	if (lua_isstring(L, 1)) {
		const char *address_string = lua_tostring(L, 1);

		AllocatedSocketAddress address;
		address.SetLocal(address_string);

		instance.AddListener(address, max_size, std::move(handler));
#ifdef HAVE_LIBSYSTEMD
	} else if (IsSystemdMagic(L, 1)) {
		instance.AddSystemdListener(max_size, std::move(handler));
#endif
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

#ifdef HAVE_LIBSODIUM
	Lua::InitSodium(L);
#endif

	Lua::InitEvent(L, instance.GetEventLoop());

#ifdef HAVE_PG
	Lua::InitPg(L, instance.GetEventLoop());
#endif

	Lua::InitSocketAddress(L);
	RegisterLuaResolver(L);

	static constexpr lua_Integer DEFAULT_MAX_SIZE = 16 * 1024 * 1024;
	Lua::SetGlobal(L, "max_size", DEFAULT_MAX_SIZE);

#ifdef HAVE_LIBSYSTEMD
	Lua::SetGlobal(L, "systemd", Lua::LightUserData(&systemd_magic));
#endif

	Lua::SetGlobal(L, "qmqp_listen",
		       Lua::MakeCClosure(l_qmqp_listen,
					 Lua::LightUserData(&instance)));
}

static void
ChdirContainingDirectory(const char *path)
{
	const char *slash = strrchr(path, '/');
	if (slash == nullptr || slash == path)
		return;

	const std::string parent{path, slash};
	if (chdir(parent.c_str()) < 0)
		throw FmtErrno("Failed to change to {}", parent);
}

static void
LoadConfigFile(lua_State *L, const char *path)
{
	ChdirContainingDirectory(path);
	Lua::RunFile(L, path);

	if (chdir("/") < 0)
		throw FmtErrno("Failed to change to {}", "/");
}

static void
SetupRuntimeState(lua_State *L)
{
	Lua::SetGlobal(L, "max_size", nullptr);
	Lua::SetGlobal(L, "qmqp_listen", nullptr);

	Lua::InitXattrTable(L);
	Lua::RegisterCgroupInfo(L);

	QmqpRelayConnection::Register(L);

	UnregisterLuaResolver(L);
}

static int
Run(const CommandLine &cmdline)
{
	Instance instance;
	SetupConfigState(instance.GetLuaState(), instance);

	LoadConfigFile(instance.GetLuaState(), cmdline.config_path.c_str());

	instance.Check();

	SetupRuntimeState(instance.GetLuaState());

#ifdef HAVE_LIBSYSTEMD
	/* tell systemd we're ready */
	sd_notify(0, "READY=1");
#endif

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
