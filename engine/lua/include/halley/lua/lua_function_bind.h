#pragma once
#include "lua_stack_ops.h"

namespace Halley {
	class String;
	class LuaState;
	class LuaReference;

	class LuaFunctionCaller {
	public:
		static void call(LuaState& state, int nArgs, int nRets);
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
		static void call(LuaState& state, int nRets)
		{
			_doCall(state, 0, nRets);
		}
		
		static void _doCall(LuaState& state, int nArgs, int nRets)
		{
			LuaFunctionCaller::call(state, nArgs, nRets);
		}
	};

	template <typename U, typename... Us>
	class LuaFunctionBind<U, Us...> {
	public:
		static void call(LuaState& state, int nRets, U u, Us... us)
		{
			_doCall(state, 0, nRets, u, us...);
		}

		static void _doCall(LuaState& state, int nArgs, int nRets, U u, Us... us)
		{
			LuaStackOps(state).push(u);
			LuaFunctionBind<Us...>::_doCall(state, nArgs + 1, nRets, us...);
		}
	};

	namespace LuaCallbackBindDetails {
		template <signed int pos, typename Tuple, std::enable_if_t<pos == 0, int> = 0>
		void doFillTuple(LuaState& state, Tuple& tuple)
		{
		}

		template <signed int pos, typename Tuple, std::enable_if_t<pos != 0, int> = 0>
		void doFillTuple(LuaState& state, Tuple& tuple)
		{
			using T = typename std::tuple_element<pos - 1, Tuple>::type;
			std::get<pos - 1>(tuple) = LuaStackReturn(state).operator T();
			doFillTuple<pos - 1, Tuple>(state, tuple);
		}
		
		template <typename... Ps>
		std::tuple<Ps...> makeTuple(LuaState& state)
		{
			std::tuple<Ps...> tuple;
			doFillTuple<sizeof...(Ps), std::tuple<Ps...>>(state, tuple);
			return tuple;
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

		template <typename T, typename R, typename... Ps>
		LuaCallback bind(T* obj, R (T::*f)(Ps...), std::enable_if_t<std::is_void<R>::value, int>)
		{
			return [=] (LuaState& state) -> int
			{
				call(obj, f, makeTuple<Ps...>(state));
				return 0;
			};
		}

		template <typename T, typename R, typename... Ps>
		LuaCallback bind(T* obj, R (T::*f)(Ps...), std::enable_if_t<!std::is_void<R>::value, int>)
		{
			return [=] (LuaState& state) -> int
			{
				R result = call(obj, f, makeTuple<Ps...>(state));
				LuaStackOps(state).push(result);
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
