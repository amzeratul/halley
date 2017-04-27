#include "lua_function_bind.h"
#include "lua_state.h"

void Halley::LuaFunctionCaller::call(LuaState& state, int nArgs, int nRets)
{
	state.call(nArgs, nRets);
}
