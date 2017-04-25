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

	namespace LuaCallbackBind {
		template <signed int pos, typename Tuple, std::enable_if_t<pos == 0, int> = 0>
		void doFillTuple(LuaState& state, Tuple& tuple)
		{
		}

		template <signed int pos, typename Tuple, std::enable_if_t<pos != 0, int> = 0>
		void doFillTuple(LuaState& state, Tuple& tuple)
		{
			std::get<pos - 1>(tuple) = LuaStackReturn(state);
			doFillTuple<pos - 1, Tuple>(state, tuple);
		}

		template <typename... Ps>
		void fillTuple(LuaState& state, std::tuple<Ps...>& tuple)
		{
			doFillTuple<sizeof...(Ps), decltype(tuple)>(state, tuple);
		}

		template <typename T, typename R, typename... Ps, typename... As>
		R applyTuple(std::enable_if_t<sizeof...(Ps) == sizeof...(As), int>, T* obj, R (T::*f)(Ps...), std::tuple<Ps...>&& tuple, As... args)
		{
			return (obj->*f)(args...);
		}

		template <typename T, typename R, typename... Ps, typename... As>
		R applyTuple(std::enable_if_t<sizeof...(Ps) != sizeof...(As), int>, T* obj, R (T::*f)(Ps...), std::tuple<Ps...>&& tuple, As... args)
		{
			return applyTuple(0, obj, f, std::move(tuple), args..., std::get<sizeof...(As)>(tuple));
		}

		template <typename T, typename R, typename... Ps>
		R call(T* obj, R (T::*f)(Ps...), std::tuple<Ps...>&& args)
		{
			return applyTuple(0, obj, f, std::move(args));
		}

		using LuaCallback = std::function<int(LuaState&)>;

		template <typename T, typename R, typename... Ps>
		LuaCallback bindCallback(T* obj, R (T::*f)(Ps...))
		{
			return [=] (LuaState& state) -> int
			{
				std::tuple<Ps...> args;
				fillTuple<Ps...>(state, args);
				R result = call(obj, f, std::move(args));
				LuaStackOps(state).push(result);
				return 1;
			};
		}
	}
}
