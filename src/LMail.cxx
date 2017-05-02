/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "LMail.hxx"
#include "MutableMail.hxx"
#include "LAction.hxx"
#include "Action.hxx"
#include "LAddress.hxx"
#include "lua/Class.hxx"
#include "CgroupProc.hxx"
#include "MountProc.hxx"

#include <sys/socket.h>
#include <string.h>

class IncomingMail : public MutableMail {
    const int fd;

    bool have_cred = false;
    struct ucred cred;

public:
    IncomingMail(MutableMail &&src, int _fd)
        :MutableMail(std::move(src)), fd(_fd) {}

    int GetPid() {
        LoadPeerCred();
        return cred.pid;
    }

    int GetUid() {
        LoadPeerCred();
        return cred.uid;
    }

    int GetGid() {
        LoadPeerCred();
        return cred.gid;
    }

private:
    void LoadPeerCred() {
        if (have_cred)
            return;

        have_cred = true;

        socklen_t len = sizeof(cred);
        if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cred, &len) < 0) {
            cred.pid = -1;
            cred.uid = -1;
            cred.gid = -1;
        }
    }
};

static constexpr char lua_mail_class[] = "qrelay.mail";
typedef Lua::Class<IncomingMail, lua_mail_class> LuaMail;

static int
InsertHeader(lua_State *L)
{
    if (lua_gettop(L) != 3)
      return luaL_error(L, "Invalid parameters");

    if (!lua_isstring(L, 2))
        luaL_argerror(L, 2, "string expected");

    if (!lua_isstring(L, 3))
        luaL_argerror(L, 3, "string expected");

    auto &mail = (IncomingMail &)CastLuaMail(L, 1);
    mail.InsertHeader(lua_tostring(L, 2), lua_tostring(L, 3));
    return 0;
}

static int
NewConnectAction(lua_State *L)
{
    if (lua_gettop(L) != 2)
      return luaL_error(L, "Invalid parameters");

    AllocatedSocketAddress address;

    try {
        address = GetLuaAddress(L, 2);
    } catch (const std::exception &e) {
        return luaL_error(L, e.what());
    }

    auto &action = *NewLuaAction(L, 1);
    action.type = Action::Type::CONNECT;
    action.connect = std::move(address);
    return 1;
}

static int
NewDiscardAction(lua_State *L)
{
    if (lua_gettop(L) != 1)
      return luaL_error(L, "Invalid parameters");

    auto &action = *NewLuaAction(L, 1);
    action.type = Action::Type::DISCARD;
    return 1;
}

static int
NewRejectAction(lua_State *L)
{
    if (lua_gettop(L) != 1)
      return luaL_error(L, "Invalid parameters");

    auto &action = *NewLuaAction(L, 1);
    action.type = Action::Type::REJECT;
    return 1;
}

static int
NewExecAction(lua_State *L)
{
    if (lua_gettop(L) < 2)
      return luaL_error(L, "Not enough parameters");

    auto &action = *NewLuaAction(L, 1);
    action.type = Action::Type::EXEC;

    const unsigned n = lua_gettop(L);
    for (unsigned i = 2; i <= n; ++i) {
        if (!lua_isstring(L, i))
            luaL_argerror(L, i, "string expected");

        action.exec.emplace_back(lua_tostring(L, i));
    }

    return 1;
}

static int
GetCgroup(lua_State *L)
try {
    if (lua_gettop(L) != 2)
      return luaL_error(L, "Invalid parameters");

    auto &mail = (IncomingMail &)CastLuaMail(L, 1);

    if (!lua_isstring(L, 2))
        luaL_argerror(L, 2, "string expected");
    const char *controller = lua_tostring(L, 2);

    const int pid = mail.GetPid();
    if (pid < 0)
        return 0;

    const auto path = ReadProcessCgroup(pid, controller);
    if (path.empty())
        return 0;

    Lua::Push(L, path.c_str());
    return 1;
} catch (const std::runtime_error &e) {
    return luaL_error(L, e.what());
}

static int
GetMountInfo(lua_State *L)
try {
    if (lua_gettop(L) != 2)
      return luaL_error(L, "Invalid parameters");

    auto &mail = (IncomingMail &)CastLuaMail(L, 1);

    if (!lua_isstring(L, 2))
        luaL_argerror(L, 2, "string expected");
    const char *mountpoint = lua_tostring(L, 2);

    const int pid = mail.GetPid();
    if (pid < 0)
        return 0;

    const auto m = ReadProcessMount(pid, mountpoint);
    if (!m.IsDefined())
        return 0;

    lua_newtable(L);
    Lua::SetField(L, -2, "root", m.root.c_str());
    Lua::SetField(L, -2, "filesystem", m.filesystem.c_str());
    Lua::SetField(L, -2, "source", m.source.c_str());
    return 1;
} catch (const std::runtime_error &e) {
    return luaL_error(L, e.what());
}

static constexpr struct luaL_reg mail_methods [] = {
    {"insert_header", InsertHeader},
    {"connect", NewConnectAction},
    {"discard", NewDiscardAction},
    {"reject", NewRejectAction},
    {"exec", NewExecAction},
    {"get_cgroup", GetCgroup},
    {"get_mount_info", GetMountInfo},
    {nullptr, nullptr}
};

static void
PushArray(lua_State *L, const std::vector<StringView> &src)
{
    lua_createtable(L, src.size(), 0);

    int i = 1;
    for (const auto &value : src)
        Lua::SetTable(L, -3, i++, value);
}

static void
PushOptional(lua_State *L, int value)
{
    if (value >= 0)
        Lua::Push(L, value);
    else
        Lua::Push(L, nullptr);
}

static int
LuaMailIndex(lua_State *L)
{
    if (lua_gettop(L) != 2)
        return luaL_error(L, "Invalid parameters");

    auto &mail = (IncomingMail &)CastLuaMail(L, 1);

    if (!lua_isstring(L, 2))
        luaL_argerror(L, 2, "string expected");

    const char *name = lua_tostring(L, 2);

    for (const auto *i = mail_methods; i->name != nullptr; ++i) {
        if (strcmp(i->name, name) == 0) {
            Lua::Push(L, i->func);
            return 1;
        }
    }

    if (strcmp(name, "sender") == 0) {
        Lua::Push(L, mail.sender);
        return 1;
    } else if (strcmp(name, "recipients") == 0) {
        PushArray(L, mail.recipients);
        return 1;
    } else if (strcmp(name, "pid") == 0) {
        PushOptional(L, mail.GetPid());
        return 1;
    } else if (strcmp(name, "uid") == 0) {
        PushOptional(L, mail.GetUid());
        return 1;
    } else if (strcmp(name, "gid") == 0) {
        PushOptional(L, mail.GetGid());
        return 1;
    }

    return luaL_error(L, "Unknown attribute");
}

void
RegisterLuaMail(lua_State *L)
{
    LuaMail::Register(L);
    Lua::SetTable(L, -3, "__index", LuaMailIndex);
    lua_pop(L, 1);
}

MutableMail *
NewLuaMail(lua_State *L, MutableMail &&src, int fd)
{
    return LuaMail::New(L, std::move(src), fd);
}

MutableMail &
CastLuaMail(lua_State *L, int idx)
{
    return LuaMail::Cast(L, idx);
}
