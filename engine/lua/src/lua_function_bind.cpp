#include <lua.hpp>
#include "lua_function_bind.h"
#include "lua_reference.h"
#include "lua_state.h"

using namespace Halley;

void LuaFunctionCaller::call(LuaReference& ref, int nArgs, int nRets)
{
	ref.call(nArgs, nRets);
}

void LuaFunctionCaller::push(LuaState& state, bool v)
{
	lua_pushboolean(state.getRawState(), v);
}

void LuaFunctionCaller::push(LuaState& state, int v)
{
	lua_pushinteger(state.getRawState(), v);
}

void LuaFunctionCaller::push(LuaState& state, const String& v)
{
	lua_pushstring(state.getRawState(), v.c_str());
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
