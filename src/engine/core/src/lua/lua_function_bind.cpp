#include "halley/lua/lua_function_bind.h"
#include "halley/lua/lua_state.h"

void Halley::LuaFunctionCaller::startCall(LuaState& state)
{
	//state.pushErrorHandler();
}

bool Halley::LuaFunctionCaller::call(LuaState& state, int nArgs, int nRets, bool throwOnError)
{
	return state.call(nArgs, nRets, throwOnError);
}

void Halley::LuaFunctionCaller::endCall(LuaState& state)
{
	//state.popErrorHandler();
}
