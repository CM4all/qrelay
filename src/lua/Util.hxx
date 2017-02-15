/*
 * Copyright (C) 2015-2017 Max Kellermann <max.kellermann@gmail.com>
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

#ifndef LUA_UTIL_HXX
#define LUA_UTIL_HXX

#include "util/StringView.hxx"

#include <inline/compiler.h>

extern "C" {
#include <lua.h>
}

#include <cstddef>
#include <tuple>

namespace Lua {

/**
 * This type represents an index on the Lua stack.
 */
struct StackIndex {
	int idx;

	constexpr StackIndex(int _idx):idx(_idx) {}
};

struct LightUserData {
	void *value;

	constexpr LightUserData(void *_value):value(_value) {}
};

template<typename... T>
struct CClosure {
	lua_CFunction fn;

	std::tuple<T...> values;
};

template<typename... T>
CClosure<T...>
MakeCClosure(lua_CFunction fn, T&&... values)
{
	return {fn, {values...}};
}

static inline void
Push(lua_State *L, std::nullptr_t)
{
	lua_pushnil(L);
}

static inline void
Push(lua_State *L, StackIndex i)
{
	lua_pushvalue(L, i.idx);
}

gcc_nonnull_all
static inline void
Push(lua_State *L, bool value)
{
	lua_pushboolean(L, value);
}

gcc_nonnull_all
static inline void
Push(lua_State *L, const char *value)
{
	lua_pushstring(L, value);
}

gcc_nonnull_all
static inline void
Push(lua_State *L, StringView value)
{
	lua_pushlstring(L, value.data, value.size);
}

gcc_nonnull_all
static inline void
Push(lua_State *L, int value)
{
	lua_pushinteger(L, value);
}

gcc_nonnull_all
static inline void
Push(lua_State *L, double value)
{
	lua_pushnumber(L, value);
}

template<std::size_t i>
struct _PushTuple {
	template<typename T>
	static void PushTuple(lua_State *L, const T &t) {
		_PushTuple<i - 1>::template PushTuple<T>(L, t);
		Push(L, std::get<i - 1>(t));
	}
};

template<>
struct _PushTuple<0> {
	template<typename T>
	static void PushTuple(lua_State *, const T &) {
	}
};

template<typename... T>
gcc_nonnull_all
void
Push(lua_State *L, const std::tuple<T...> &t)
{
	_PushTuple<sizeof...(T)>::template PushTuple<std::tuple<T...>>(L, t);
}

gcc_nonnull_all
static inline void
Push(lua_State *L, lua_CFunction value)
{
	lua_pushcfunction(L, value);
}

template<typename... T>
gcc_nonnull_all
void
Push(lua_State *L, const CClosure<T...> &value)
{
	Push(L, value.values);
	lua_pushcclosure(L, value.fn, sizeof...(T));
}

gcc_nonnull_all
static inline void
Push(lua_State *L, LightUserData value)
{
	lua_pushlightuserdata(L, value.value);
}

template<typename V>
void
SetGlobal(lua_State *L, const char *name, V &&value)
{
	Push(L, std::forward<V>(value));
	lua_setglobal(L, name);
}

template<typename K>
void
GetTable(lua_State *L, int idx, K &&key)
{
	Push(L, std::forward<K>(key));
	lua_gettable(L, idx);
}

template<typename K, typename V>
void
SetTable(lua_State *L, int idx, K &&key, V &&value)
{
	Push(L, std::forward<K>(key));
	Push(L, std::forward<V>(value));
	lua_settable(L, idx);
}

template<typename V>
void
SetField(lua_State *L, int idx, const char *name, V &&value)
{
	Push(L, std::forward<V>(value));
	lua_setfield(L, idx, name);
}

template<typename V>
static inline void
SetRegistry(lua_State *L, const char *name, V &&value)
{
	SetField(L, LUA_REGISTRYINDEX, name, std::forward<V>(value));
}

static inline void *
GetRegistryLightUserData(lua_State *L, const char *name)
{
	lua_getfield(L, LUA_REGISTRYINDEX, name);
	void *value = lua_touserdata(L, -1);
	lua_pop(L, 1);
	return value;
}

template<typename V>
static inline void
SetField(lua_State *L, const char *package, const char *name, V &&value)
{
	lua_getglobal(L, package);
	SetField(L, -2, name, std::forward<V>(value));
	lua_pop(L, 1);
}

/**
 * Sets the variable "package.path", which controls the package
 * search path for the "require" command.
 */
gcc_nonnull_all
static inline void
SetPackagePath(lua_State *L, const char *path) {
	SetField(L, "package", "path", path);
}

template<typename F>
void ForEach(lua_State *L) {
}

}

#endif
