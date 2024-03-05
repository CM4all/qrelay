// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "LMail.hxx"
#include "MutableMail.hxx"
#include "LAction.hxx"
#include "Action.hxx"
#include "lua/AutoCloseList.hxx"
#include "lua/Class.hxx"
#include "lua/Error.hxx"
#include "lua/FenvCache.hxx"
#include "lua/ForEach.hxx"
#include "lua/StringView.hxx"
#include "lua/io/CgroupInfo.hxx"
#include "lua/net/SocketAddress.hxx"
#include "uri/EmailAddress.hxx"
#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"
#include "io/Beneath.hxx"
#include "io/FileAt.hxx"
#include "io/Open.hxx"
#include "io/UniqueFileDescriptor.hxx"
#include "io/linux/ProcCgroup.hxx"

extern "C" {
#include <lauxlib.h>
}

#include <fmt/core.h>

#include <sys/socket.h>
#include <string.h>

using std::string_view_literals::operator""sv;

class IncomingMail : public MutableMail {
	Lua::AutoCloseList *auto_close;

	const struct ucred cred;

public:
	IncomingMail(lua_State *L, Lua::AutoCloseList &_auto_close,
		     MutableMail &&src, const struct ucred &_peer_cred)
		:MutableMail(std::move(src)),
		 auto_close(&_auto_close),
		 cred(_peer_cred)
	{
		auto_close->Add(L, Lua::RelativeStackIndex{-1});

		lua_newtable(L);
		lua_setfenv(L, -2);
	}

	bool IsStale() const noexcept {
		return auto_close == nullptr;
	}

	bool HavePeerCred() const noexcept {
		return cred.pid >= 0;
	}

	int Close(lua_State *) {
		auto_close = nullptr;
		return 0;
	}

	int Index(lua_State *L);
	int NewIndex(lua_State *L);
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

/**
 * Collect parameters from the "options" table passed as the last
 * parameter to exec() / exec_raw().
 */
static void
CollectExecEnv(Action &action, lua_State *L, Lua::AnyStackIndex auto idx)
{
	Lua::ForEach(L, idx, [L, &action](auto name_idx, auto value_idx){
		if (action.exec.full())
			luaL_error(L, "Too many environment variables");

		if (!lua_isstring(L, Lua::GetStackIndex(name_idx)))
			luaL_error(L, "Environment name is not a string");
		if (!lua_isstring(L, Lua::GetStackIndex(value_idx)))
			luaL_error(L, "Environment value is not a string");

		const auto name = Lua::ToStringView(L, Lua::GetStackIndex(name_idx));
		const auto value = Lua::ToStringView(L, Lua::GetStackIndex(value_idx));
		action.env.emplace_back(fmt::format("{}={}", name, value));
	});
}

/**
 * Collect parameters from the "options" table passed as the last
 * parameter to exec() / exec_raw().
 */
static void
CollectExecOptions(Action &action, lua_State *L, Lua::AnyStackIndex auto idx)
{
	Lua::ForEach(L, idx, [L, &action](auto key_idx, auto value_idx){
		if (!lua_isstring(L, Lua::GetStackIndex(key_idx)))
			luaL_error(L, "Option key is not a string");

		const auto key = Lua::ToStringView(L, Lua::GetStackIndex(key_idx));
		if (key == "env"sv)
			CollectExecEnv(action, L, value_idx);
		else
			luaL_error(L, "Unknown option");
	});
}

/**
 * Collect exec() / exec_raw() parameters and store them in an
 * #Action.
 */
static void
CollectExec(Action &action, lua_State *L, unsigned top)
{
	/* if the last parameter is a table, it may contain options */
	if (lua_istable(L, top)) {
		CollectExecOptions(action, L, Lua::StackIndex(top));
		--top;
	}

	for (unsigned i = 2; i <= top; ++i) {
		if (action.exec.full())
			luaL_argerror(L, i, "Too many arguments");

		action.exec.emplace_back(luaL_checkstring(L, i));
	}
}

static int
NewExecAction(lua_State *L)
{
	const unsigned top = lua_gettop(L);
	if (top < 2)
		return luaL_error(L, "Not enough parameters");

	auto &action = *NewLuaAction(L, 1);
	action.type = Action::Type::EXEC;

	CollectExec(action, L, top);

	return 1;
}

static int
NewExecRawAction(lua_State *L)
{
	const unsigned top = lua_gettop(L);
	if (top < 2)
		return luaL_error(L, "Not enough parameters");

	auto &action = *NewLuaAction(L, 1);
	action.type = Action::Type::EXEC_RAW;

	CollectExec(action, L, top);

	return 1;
}

static constexpr struct luaL_Reg mail_methods [] = {
	{"insert_header", InsertHeader},
	{"connect", NewConnectAction},
	{"discard", NewDiscardAction},
	{"reject", NewRejectAction},
	{"exec", NewExecAction},
	{"exec_raw", NewExecRawAction},
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
IncomingMail::Index(lua_State *L)
{
	if (lua_gettop(L) != 2)
		return luaL_error(L, "Invalid parameters");

	constexpr Lua::StackIndex name_idx{2};
	const char *const name = luaL_checkstring(L, 2);

	if (IsStale())
		return luaL_error(L, "Stale object");

	for (const auto *i = mail_methods; i->name != nullptr; ++i) {
		if (strcmp(i->name, name) == 0) {
			Lua::Push(L, i->func);
			return 1;
		}
	}

	// look it up in the fenv (our cache)
	if (Lua::GetFenvCache(L, 1, name_idx))
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

		Lua::Push(L, static_cast<lua_Integer>(cred.pid));
		return 1;
	} else if (strcmp(name, "uid") == 0) {
		if (!HavePeerCred())
			return 0;

		Lua::Push(L, static_cast<lua_Integer>(cred.uid));
		return 1;
	} else if (strcmp(name, "gid") == 0) {
		if (!HavePeerCred())
			return 0;

		Lua::Push(L, static_cast<lua_Integer>(cred.gid));
		return 1;
	} else if (StringIsEqual(name, "cgroup")) {
		if (!HavePeerCred())
			return 0;

		const auto path = ReadProcessCgroup(cred.pid);
		if (path.empty())
			return 0;

		Lua::NewCgroupInfo(L, *auto_close, path);

		// copy a reference to the fenv (our cache)
		Lua::SetFenvCache(L, 1, name_idx, Lua::RelativeStackIndex{-1});

		return 1;
	} else
		return luaL_error(L, "Unknown attribute");
}

inline int
IncomingMail::NewIndex(lua_State *L)
{
	if (lua_gettop(L) != 3)
		return luaL_error(L, "Invalid parameters");

	if (IsStale())
		return luaL_error(L, "Stale object");

	const char *name = luaL_checkstring(L, 2);
	if (StringIsEqual(name, "sender")) {
		const char *new_value = luaL_checkstring(L, 3);
		if (!VerifyEmailAddress(new_value))
			luaL_argerror(L, 3, "Malformed email address");

		SetSender(new_value);
		return 0;
	} else
		return luaL_error(L, "Cannot change this attribute");
}

void
RegisterLuaMail(lua_State *L)
{
	using namespace Lua;

	LuaMail::Register(L);
	SetField(L, RelativeStackIndex{-1}, "__close", LuaMail::WrapMethod<&IncomingMail::Close>());
	SetField(L, RelativeStackIndex{-1}, "__index", LuaMail::WrapMethod<&IncomingMail::Index>());
	SetField(L, RelativeStackIndex{-1}, "__newindex", LuaMail::WrapMethod<&IncomingMail::NewIndex>());
	lua_pop(L, 1);
}

MutableMail *
NewLuaMail(lua_State *L,
	   Lua::AutoCloseList &auto_close,
	   MutableMail &&src, const struct ucred &peer_cred)
{
	return LuaMail::New(L, L, auto_close, std::move(src), peer_cred);
}

MutableMail &
CastLuaMail(lua_State *L, int idx)
{
	return LuaMail::Cast(L, idx);
}
