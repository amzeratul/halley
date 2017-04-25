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
	int result = lua_pcall(state.getRawState(), nArgs, nRets, 0);
	if (result != 0) {
		throw Exception("Lua exception:\n\t" + LuaStackOps(state).popString());
	}
}

