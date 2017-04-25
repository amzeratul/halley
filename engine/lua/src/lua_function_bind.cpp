#include <lua.hpp>
#include "lua_function_bind.h"
#include "lua_reference.h"
#include "lua_state.h"

using namespace Halley;

void LuaFunctionCaller::startCall(LuaReference& ref)
{
	ref.pushToLuaStack();
}

void LuaFunctionCaller::endCall(LuaState& state, int nArgs, int nRets)
{
	lua_pcall(state.getRawState(), nArgs, nRets, 0);
}

void LuaFunctionCaller::push(LuaState& state, nullptr_t)
{
	lua_pushnil(state.getRawState());
}

void LuaFunctionCaller::push(LuaState& state, bool v)
{
	lua_pushboolean(state.getRawState(), v);
}

void LuaFunctionCaller::push(LuaState& state, int v)
{
	lua_pushinteger(state.getRawState(), v);
}

void LuaFunctionCaller::push(LuaState& state, int64_t v)
{
	lua_pushinteger(state.getRawState(), v);
}

void LuaFunctionCaller::push(LuaState& state, double v)
{
	lua_pushnumber(state.getRawState(), v);
}

void LuaFunctionCaller::push(LuaState& state, const String& v)
{
	lua_pushstring(state.getRawState(), v.c_str());
}

void LuaFunctionCaller::push(LuaState& state, Vector2i v)
{
	lua_createtable(state.getRawState(), 0, 2);
	push(state, v.x);
	lua_setfield(state.getRawState(), -2, "x");
	push(state, v.y);
	lua_setfield(state.getRawState(), -2, "y");
}

LuaStackReturn::LuaStackReturn(LuaState& state)
	: state(state)
{
}

LuaStackReturn::operator bool() const
{
	auto value = lua_toboolean(state.getRawState(), -1) != 0;
	lua_pop(state.getRawState(), 1);
	return value;
}

LuaStackReturn::operator int() const
{
	auto value = lua_tointeger(state.getRawState(), -1);
	lua_pop(state.getRawState(), 1);
	return int(value);
}

LuaStackReturn::operator int64_t() const
{
	auto value = lua_tointeger(state.getRawState(), -1);
	lua_pop(state.getRawState(), 1);
	return value;
}

LuaStackReturn::operator double() const
{
	auto value = lua_tonumber(state.getRawState(), -1);
	lua_pop(state.getRawState(), 1);
	return value;
}

LuaStackReturn::operator String() const
{
	String value = lua_tostring(state.getRawState(), -1);
	lua_pop(state.getRawState(), 1);
	return value;
}

LuaStackReturn::operator Vector2i() const
{
	if (!lua_istable(state.getRawState(), -1)) {
		throw Exception("Invalid value at Lua stack");
	}
	Vector2i result;
	lua_getfield(state.getRawState(), -1, "x");
	result.x = *this;
	lua_getfield(state.getRawState(), -1, "y");
	result.y = *this;
	return result;
}
