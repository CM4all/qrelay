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

#include "LMail.hxx"
#include "MutableMail.hxx"
#include "LAction.hxx"
#include "Action.hxx"
#include "LAddress.hxx"
#include "lua/Class.hxx"
#include "lua/Error.hxx"
#include "CgroupProc.hxx"
#include "MountProc.hxx"

#include <sys/socket.h>
#include <string.h>

class IncomingMail : public MutableMail {
	const struct ucred cred;

public:
	IncomingMail(MutableMail &&src, const struct ucred &_peer_cred)
		:MutableMail(std::move(src)), cred(_peer_cred) {}

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

	if (!lua_isstring(L, 2))
		luaL_argerror(L, 2, "string expected");

	const char *name = lua_tostring(L, 2);
	if (!IsValidHeaderName(name))
		luaL_argerror(L, 2, "Illegal header name");

	if (!lua_isstring(L, 3))
		luaL_argerror(L, 3, "string expected");

	const char *value = lua_tostring(L, 3);
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
		address = GetLuaAddress(L, 2);
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

	if (!mail.HavePeerCred())
		return 0;

	const auto path = ReadProcessCgroup(mail.GetPid(), controller);
	if (path.empty())
		return 0;

	Lua::Push(L, path.c_str());
	return 1;
} catch (...) {
	Lua::RaiseCurrent(L);
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

	if (!mail.HavePeerCred())
		return 0;

	const auto m = ReadProcessMount(mail.GetPid(), mountpoint);
	if (!m.IsDefined())
		return 0;

	lua_newtable(L);
	Lua::SetField(L, -2, "root", m.root.c_str());
	Lua::SetField(L, -2, "filesystem", m.filesystem.c_str());
	Lua::SetField(L, -2, "source", m.source.c_str());
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
		if (!mail.HavePeerCred())
			return 0;

		Lua::Push(L, mail.GetPid());
		return 1;
	} else if (strcmp(name, "uid") == 0) {
		if (!mail.HavePeerCred())
			return 0;

		Lua::Push(L, mail.GetUid());
		return 1;
	} else if (strcmp(name, "gid") == 0) {
		if (!mail.HavePeerCred())
			return 0;

		Lua::Push(L, mail.GetGid());
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
NewLuaMail(lua_State *L, MutableMail &&src, const struct ucred &peer_cred)
{
	return LuaMail::New(L, std::move(src), peer_cred);
}

MutableMail &
CastLuaMail(lua_State *L, int idx)
{
	return LuaMail::Cast(L, idx);
}
