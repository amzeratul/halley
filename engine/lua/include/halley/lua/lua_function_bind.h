#pragma once

namespace Halley {
	class String;
	class LuaState;
	class LuaReference;

	class LuaFunctionCaller {
	public:
		static void call(LuaReference& ref, int nArgs, int nRets);
		static void push(LuaState& state, bool v);
		static void push(LuaState& state, int v);
		static void push(LuaState& state, const String& v);
	};

	class LuaStackReturn {
	public:
		explicit LuaStackReturn(LuaState& state);

		operator bool() const;

	private:
		LuaState& state;
	};

	template <typename... Us>
	class LuaFunctionBind;

	template <>
	class LuaFunctionBind<> {
	public:
		static LuaStackReturn call(LuaState& state, LuaReference& ref)
		{
			return doCall(state, ref, 0);
		}
		
		static LuaStackReturn doCall(LuaState& state, LuaReference& ref, int nArgs)
		{
			LuaFunctionCaller::call(ref, nArgs, 1);
			return LuaStackReturn(state);
		}
	};

	template <typename U, typename... Us>
	class LuaFunctionBind<U, Us...> {
	public:
		static LuaStackReturn call(LuaState& state, LuaReference& ref, U u, Us... us)
		{
			return doCall(state, ref, 0, u, us...);
		}

		static LuaStackReturn doCall(LuaState& state, LuaReference& ref, int nArgs, U u, Us... us)
		{
			LuaFunctionCaller::push(state, u);
			return LuaFunctionBind<Us...>::doCall(state, ref, nArgs + 1, us...);
		}
	};
}
