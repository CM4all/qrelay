/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "CommandLine.hxx"
#include "Instance.hxx"
#include "system/SetupProcess.hxx"
#include "net/AllocatedSocketAddress.hxx"
#include "lua/State.hxx"
#include "lua/Value.hxx"
#include "lua/Util.hxx"
#include "lua/RunFile.hxx"
#include "util/OstreamException.hxx"

#include <daemon/daemonize.h>

extern "C" {
#include <lauxlib.h>
#include <lualib.h>
}

#include <iostream>
using std::cerr;
using std::endl;

#include <stdlib.h>

static int
l_qmqp_listen(lua_State *L)
{
  if (lua_gettop(L) != 2)
      return luaL_error(L, "Invalid parameter count");

  if (!lua_isstring(L, 1))
      luaL_argerror(L, 1, "path expected");

  if (!lua_isfunction(L, 2))
      luaL_argerror(L, 2, "function expected");

  const char *address_string = lua_tostring(L, 1);
  AllocatedSocketAddress address;
  address.SetLocal(address_string);

  auto handler = std::make_shared<Lua::Value>(L, 2);

  auto &instance = *(Instance *)lua_touserdata(L, lua_upvalueindex(1));
  instance.AddQmqpRelayServer(address, L, std::move(handler));

  return 0;
}

static void
SetupState(lua_State *L, Instance &instance)
{
    luaL_openlibs(L);

    Lua::SetGlobal(L, "qmqp_listen",
                   Lua::MakeCClosure(l_qmqp_listen,
                                     Lua::LightUserData(&instance)));

    QmqpRelayConnection::Register(L);
}

static int
Run(const CommandLine &cmdline)
{
    Lua::State state(luaL_newstate());

    Instance instance;
    SetupState(state.get(), instance);

    Lua::RunFile(state.get(), cmdline.config_path.c_str());

    instance.Check();

    if (daemonize() < 0)
        return EXIT_FAILURE;

    instance.GetEventLoop().Dispatch();

    return EXIT_SUCCESS;
}

int
main(int argc, char **argv)
try {
    daemon_config.detach = false;

    const auto cmdline = ParseCommandLine(argc, argv);

    SetupProcess();

    const int result = Run(cmdline);

    daemonize_cleanup();

    cerr << "exiting" << endl;
    return result;
} catch (const std::exception &e) {
    cerr << "error: " << e << endl;
    return EXIT_FAILURE;
}
