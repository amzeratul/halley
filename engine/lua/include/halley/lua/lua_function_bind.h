#pragma once
#include "lua_stack_ops.h"

namespace Halley {
	class String;
	class LuaState;
	class LuaReference;

	class LuaFunctionCaller {
	public:
		static void startCall(LuaReference& ref);
		static void endCall(LuaState& state, int nArgs, int nRets);
	};

	template <typename T>
	struct LuaReturnSize {
		static constexpr int value = 1;
	};

	template <>
	struct LuaReturnSize<void> {
		static constexpr int value = 0;
	};

	template <typename T>
	struct LuaConvert {
		static T fromStack(LuaState& state) { return T(LuaStackReturn(state)); }
	};

	template <>
	struct LuaConvert<void> {
		static void fromStack(LuaState&) {}
	};

	template <typename... Us>
	class LuaFunctionBind;

	template <>
	class LuaFunctionBind<> {
	public:
		static void call(LuaState& state, LuaReference& ref, int nRets)
		{
			LuaFunctionCaller::startCall(ref);
			_doCall(state, 0, nRets);
		}
		
		static void _doCall(LuaState& state, int nArgs, int nRets)
		{
			LuaFunctionCaller::endCall(state, nArgs, nRets);
		}
	};

	template <typename U, typename... Us>
	class LuaFunctionBind<U, Us...> {
	public:
		static void call(LuaState& state, LuaReference& ref, int nRets, U u, Us... us)
		{
			LuaFunctionCaller::startCall(ref);
			_doCall(state, 0, nRets, u, us...);
		}

		static void _doCall(LuaState& state, int nArgs, int nRets, U u, Us... us)
		{
			LuaStackOps(state).push(u);
			LuaFunctionBind<Us...>::_doCall(state, nArgs + 1, nRets, us...);
		}
	};
}
