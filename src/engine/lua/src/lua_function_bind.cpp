#include "lua_function_bind.h"
#include "lua_state.h"

void Halley::LuaFunctionCaller::startCall(LuaState& state)
{
	state.pushErrorHandler();
}

void Halley::LuaFunctionCaller::call(LuaState& state, int nArgs, int nRets)
{
	state.call(nArgs, nRets);
}

void Halley::LuaFunctionCaller::endCall(LuaState& state)
{
	state.popErrorHandler();
}
