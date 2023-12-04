// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "LMail.hxx"
#include "MutableMail.hxx"
#include "LAction.hxx"
#include "Action.hxx"
#include "lua/Class.hxx"
#include "lua/Error.hxx"
#include "lua/FenvCache.hxx"
#include "lua/io/XattrTable.hxx"
#include "lua/net/SocketAddress.hxx"
#include "uri/EmailAddress.hxx"
#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"
#include "io/Beneath.hxx"
#include "io/FileAt.hxx"
#include "io/Open.hxx"
#include "io/UniqueFileDescriptor.hxx"
#include "io/linux/MountInfo.hxx"
#include "io/linux/ProcCgroup.hxx"

extern "C" {
#include <lauxlib.h>
}

#include <sys/socket.h>
#include <string.h>

class IncomingMail : public MutableMail {
	const struct ucred cred;

public:
	IncomingMail(lua_State *L, MutableMail &&src, const struct ucred &_peer_cred)
		:MutableMail(std::move(src)),
		 cred(_peer_cred)
	{
		lua_newtable(L);
		lua_setfenv(L, -2);
	}

	bool HavePeerCred() const noexcept {
		return cred.pid >= 0;
	}

	int GetPid() const noexcept {
		assert(HavePeerCred());

		return cred.pid;
	}

	int GetUid() const noexcept {
		assert(HavePeerCred());

		return cred.uid;
	}

	int GetGid() const noexcept{
		assert(HavePeerCred());

		return cred.gid;
	}

	int Index(lua_State *L, const char *name);
};

static constexpr char lua_mail_class[] = "qrelay.mail";
typedef Lua::Class<IncomingMail, lua_mail_class> LuaMail;

/**
 * @see RFC2822 2.2
 */
static constexpr bool
IsValidHeaderNameChar(char ch)
{
	return ch >= 33 && ch <= 126 && ch != ':';
}

/**
 * Is this a valid header name according to RFC2822 2.2?
 */
static bool
IsValidHeaderName(const char *p)
{
	do {
		if (!IsValidHeaderNameChar(*p))
			return false;
	} while (*++p != 0);

	return true;
}

/**
 * Note that this is more strict than RFC2822 2.2; only printable
 * characters are allowed.
 */
static constexpr bool
IsValidHeaderValueChar(char ch)
{
	return ch >= ' ' && ch <= 126;
}

/**
 * Is this a valid header value according to RFC2822 2.2?
 */
static bool
IsValidHeaderValue(const char *p)
{
	for (; *p != 0; ++p)
		if (!IsValidHeaderValueChar(*p))
			return false;

	return true;
}

static int
InsertHeader(lua_State *L)
{
	if (lua_gettop(L) != 3)
		return luaL_error(L, "Invalid parameters");

	const char *name = luaL_checkstring(L, 2);
	if (!IsValidHeaderName(name))
		luaL_argerror(L, 2, "Illegal header name");

	const char *value = luaL_checkstring(L, 3);
	if (!IsValidHeaderValue(value))
		luaL_argerror(L, 2, "Illegal header value");

	auto &mail = (IncomingMail &)CastLuaMail(L, 1);
	mail.InsertHeader(name, value);
	return 0;
}

static int
NewConnectAction(lua_State *L)
{
	if (lua_gettop(L) != 2)
		return luaL_error(L, "Invalid parameters");

	AllocatedSocketAddress address;

	try {
		address = Lua::ToSocketAddress(L, 2, 628);
	} catch (...) {
		Lua::RaiseCurrent(L);
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
		action.exec.emplace_back(luaL_checkstring(L, i));
	}

	return 1;
}

static int
GetMountInfo(lua_State *L)
try {
	using namespace Lua;

	if (lua_gettop(L) != 2)
		return luaL_error(L, "Invalid parameters");

	auto &mail = (IncomingMail &)CastLuaMail(L, 1);

	const char *mountpoint = luaL_checkstring(L, 2);

	if (!mail.HavePeerCred())
		return 0;

	const auto m = ReadProcessMount(mail.GetPid(), mountpoint);
	if (!m.IsDefined())
		return 0;

	lua_newtable(L);
	SetField(L, RelativeStackIndex{-1}, "root", m.root);
	SetField(L, RelativeStackIndex{-1}, "filesystem", m.filesystem);
	SetField(L, RelativeStackIndex{-1}, "source", m.source);
	return 1;
} catch (...) {
	Lua::RaiseCurrent(L);
}

static constexpr struct luaL_Reg mail_methods [] = {
	{"insert_header", InsertHeader},
	{"connect", NewConnectAction},
	{"discard", NewDiscardAction},
	{"reject", NewRejectAction},
	{"exec", NewExecAction},
	{"get_mount_info", GetMountInfo},
	{nullptr, nullptr}
};

static void
PushArray(lua_State *L, const std::vector<std::string_view> &src)
{
	using namespace Lua;

	lua_createtable(L, src.size(), 0);

	int i = 1;
	for (const auto &value : src)
		SetTable(L, RelativeStackIndex{-1},
			 static_cast<lua_Integer>(i++), value);
}

inline int
IncomingMail::Index(lua_State *L, const char *name)
{

	for (const auto *i = mail_methods; i->name != nullptr; ++i) {
		if (strcmp(i->name, name) == 0) {
			Lua::Push(L, i->func);
			return 1;
		}
	}

	// look it up in the fenv (our cache)
	if (Lua::GetFenvCache(L, 1, name))
		return 1;

	if (strcmp(name, "sender") == 0) {
		Lua::Push(L, sender);
		return 1;
	} else if (strcmp(name, "recipients") == 0) {
		PushArray(L, recipients);
		return 1;
	} else if (strcmp(name, "pid") == 0) {
		if (!HavePeerCred())
			return 0;

		Lua::Push(L, static_cast<lua_Integer>(GetPid()));
		return 1;
	} else if (strcmp(name, "uid") == 0) {
		if (!HavePeerCred())
			return 0;

		Lua::Push(L, static_cast<lua_Integer>(GetUid()));
		return 1;
	} else if (strcmp(name, "gid") == 0) {
		if (!HavePeerCred())
			return 0;

		Lua::Push(L, static_cast<lua_Integer>(GetGid()));
		return 1;
	} else if (StringIsEqual(name, "cgroup")) {
		if (!HavePeerCred())
			return 0;

		const auto path = ReadProcessCgroup(GetPid());
		if (path.empty())
			return 0;

		Lua::Push(L, path);
		return 1;
	} else if (StringIsEqual(name, "cgroup_xattr")) {
		if (!HavePeerCred())
			return 0;

		const auto path = ReadProcessCgroup(GetPid());
		if (path.empty())
			return 0;

		try {
			const auto sys_fs_cgroup = OpenPath("/sys/fs/cgroup");
			auto fd = OpenReadOnlyBeneath({sys_fs_cgroup, path.c_str() + 1});
			Lua::NewXattrTable(L, std::move(fd));
		} catch (...) {
			Lua::RaiseCurrent(L);
		}

		// copy a reference to the fenv (our cache)
		Lua::SetFenvCache(L, 1, name, Lua::RelativeStackIndex{-1});

		return 1;
	} else
		return luaL_error(L, "Unknown attribute");
}

static int
LuaMailIndex(lua_State *L)
{
	if (lua_gettop(L) != 2)
		return luaL_error(L, "Invalid parameters");

	auto &mail = (IncomingMail &)CastLuaMail(L, 1);

	const char *name = luaL_checkstring(L, 2);

	return mail.Index(L, name);
}

static int
LuaMailNewIndex(lua_State *L)
{
	if (lua_gettop(L) != 3)
		return luaL_error(L, "Invalid parameters");

	auto &mail = (IncomingMail &)CastLuaMail(L, 1);

	const char *name = luaL_checkstring(L, 2);
	if (StringIsEqual(name, "sender")) {
		const char *new_value = luaL_checkstring(L, 3);
		if (!VerifyEmailAddress(new_value))
			luaL_argerror(L, 3, "Malformed email address");

		mail.SetSender(new_value);
		return 0;
	} else
		return luaL_error(L, "Cannot change this attribute");
}

void
RegisterLuaMail(lua_State *L)
{
	using namespace Lua;

	LuaMail::Register(L);
	SetTable(L, RelativeStackIndex{-1}, "__index", LuaMailIndex);
	SetTable(L, RelativeStackIndex{-1}, "__newindex", LuaMailNewIndex);
	lua_pop(L, 1);
}

MutableMail *
NewLuaMail(lua_State *L, lua_State *main_L,
	   MutableMail &&src, const struct ucred &peer_cred)
{
	return LuaMail::New(L, main_L, std::move(src), peer_cred);
}

MutableMail &
CastLuaMail(lua_State *L, int idx)
{
	return LuaMail::Cast(L, idx);
}
