/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "CommandLine.hxx"
#include "Instance.hxx"
#include "LAddress.hxx"
#include "LResolver.hxx"
#include "system/SetupProcess.hxx"
#include "net/AllocatedSocketAddress.hxx"
#include "lua/State.hxx"
#include "lua/Value.hxx"
#include "lua/Util.hxx"
#include "lua/RunFile.hxx"
#include "util/OstreamException.hxx"

extern "C" {
#include <lauxlib.h>
#include <lualib.h>
}

#include <systemd/sd-daemon.h>

#include <iostream>
using std::cerr;
using std::endl;

#include <stdlib.h>

static int systemd_magic = 42;

static bool
IsSystemdMagic(lua_State *L, int idx)
{
    return lua_islightuserdata(L, idx) &&
        lua_touserdata(L, idx) == &systemd_magic;
}

static int
l_qmqp_listen(lua_State *L)
{
  auto &instance = *(Instance *)lua_touserdata(L, lua_upvalueindex(1));

  if (lua_gettop(L) != 2)
      return luaL_error(L, "Invalid parameter count");

  if (!lua_isfunction(L, 2))
      luaL_argerror(L, 2, "function expected");

  auto handler = std::make_shared<Lua::Value>(L, Lua::StackIndex(2));

  if (IsSystemdMagic(L, 1)) {
      instance.AddSystemdQmqpRelayServer(L, std::move(handler));
  } else if (lua_isstring(L, 1)) {
      const char *address_string = lua_tostring(L, 1);

      AllocatedSocketAddress address;
      address.SetLocal(address_string);

      instance.AddQmqpRelayServer(address, L, std::move(handler));
  } else
      luaL_argerror(L, 1, "path expected");

  return 0;
}

static void
SetupConfigState(lua_State *L, Instance &instance)
{
    luaL_openlibs(L);

    RegisterLuaAddress(L);
    RegisterLuaResolver(L);

    Lua::SetGlobal(L, "systemd", Lua::LightUserData(&systemd_magic));

    Lua::SetGlobal(L, "qmqp_listen",
                   Lua::MakeCClosure(l_qmqp_listen,
                                     Lua::LightUserData(&instance)));
}

static void
SetupRuntimeState(lua_State *L)
{
    Lua::SetGlobal(L, "qmqp_listen", nullptr);

    QmqpRelayConnection::Register(L);

    UnregisterLuaResolver(L);
}

static int
Run(const CommandLine &cmdline)
{
    Lua::State state(luaL_newstate());

    Instance instance;
    SetupConfigState(state.get(), instance);

    Lua::RunFile(state.get(), cmdline.config_path.c_str());

    instance.Check();

    SetupRuntimeState(state.get());

    /* tell systemd we're ready */
    sd_notify(0, "READY=1");

    instance.GetEventLoop().Dispatch();

    return EXIT_SUCCESS;
}

int
main(int argc, char **argv)
try {
    const auto cmdline = ParseCommandLine(argc, argv);

    SetupProcess();

    const int result = Run(cmdline);
    cerr << "exiting" << endl;
    return result;
} catch (const std::exception &e) {
    cerr << "error: " << e << endl;
    return EXIT_FAILURE;
}
