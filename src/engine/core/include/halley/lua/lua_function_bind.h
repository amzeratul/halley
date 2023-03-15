#pragma once
#include "lua_stack_ops.h"

namespace Halley {
	class String;
	class LuaState;
	class LuaReference;

	class LuaFunctionCaller {
	public:
		static void startCall(LuaState& state);
		static void call(LuaState& state, int nArgs, int nRets);
		static void endCall(LuaState& state);
	};

	template <typename T>
	struct LuaReturnSize {
		static constexpr int value = 1;
	};

	template <>
	struct LuaReturnSize<void> {
		static constexpr int value = 0;
	};

	template <typename... Us>
	class LuaFunctionBind;

	template <>
	class LuaFunctionBind<> {
	public:
		static void call(LuaState& state, int nArgs, int nRets)
		{
			LuaFunctionCaller::call(state, nArgs, nRets);
		}
	};

	template <typename U, typename... Us>
	class LuaFunctionBind<U, Us...> {
	public:
		static void call(LuaState& state, int nArgs, int nRets, U u, Us... us)
		{
			ToLua<U>()(state, u);
			LuaFunctionBind<Us...>::call(state, nArgs + 1, nRets, us...);
		}
	};

	namespace LuaCallbackBindDetails {
		template <signed int pos, typename Tuple>
		inline void doFillTuple(LuaState& state, Tuple& tuple)
		{
			if constexpr (pos != 0) {
				using T = typename std::tuple_element<pos - 1, Tuple>::type;
				std::get<pos - 1>(tuple) = FromLua<T>()(state);
				doFillTuple<pos - 1, Tuple>(state, tuple);
			}
		}
		
		template <typename... Ps>
		inline std::tuple<Ps...> makeTuple(LuaState& state)
		{
			std::tuple<Ps...> tuple;
			doFillTuple<sizeof...(Ps), std::tuple<Ps...>>(state, tuple);
			return tuple;
		}

		template <typename T, typename R, typename... Ps, typename... As>
		inline R applyTuple(std::enable_if_t<sizeof...(Ps) == sizeof...(As), T*> obj, R (T::*f)(Ps...), std::tuple<Ps...>& tuple, As... args)
		{
			return (obj->*f)(args...);
		}

		template <typename T, typename R, typename... Ps, typename... As>
		inline R applyTuple(std::enable_if_t<sizeof...(Ps) != sizeof...(As), T*> obj, R (T::*f)(Ps...), std::tuple<Ps...>& tuple, As... args)
		{
			return applyTuple(obj, f, tuple, args..., std::get<sizeof...(As)>(tuple));
		}

		template <typename T, typename R, typename... Ps>
		inline R call(T* obj, R (T::*f)(Ps...), std::tuple<Ps...>&& args)
		{
			return applyTuple(obj, f, args);
		}

		template <typename T, typename R, typename... Ps>
		inline LuaCallback bind(T* obj, R (T::*f)(Ps...), std::enable_if_t<std::is_void<R>::value, int>)
		{
			return [=] (LuaState& state) -> int
			{
				call(obj, f, makeTuple<Ps...>(state));
				return 0;
			};
		}

		template <typename T, typename R, typename... Ps>
		inline LuaCallback bind(T* obj, R (T::*f)(Ps...), std::enable_if_t<!std::is_void<R>::value, int>)
		{
			return [=] (LuaState& state) -> int
			{
				R result = call(obj, f, makeTuple<Ps...>(state));
				ToLua<R>()(state, result);
				return 1;
			};
		}
	}

	template <typename T, typename R, typename... Ps>
	LuaCallback LuaCallbackBind(T* obj, R (T::*f)(Ps...))
	{
		return LuaCallbackBindDetails::bind(obj, f, 0);
	}
}
