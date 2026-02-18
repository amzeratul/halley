#pragma once
#include "lua_stack_ops.h"

namespace Halley {
	class String;
	class LuaState;
	class LuaReference;

	class LuaFunctionCaller {
	public:
		static void startCall(LuaState& state);
		static bool call(LuaState& state, int nArgs, int nRets, bool throwOnError);
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
		static bool call(LuaState& state, int nArgs, int nRets, bool throwOnError)
		{
			return LuaFunctionCaller::call(state, nArgs, nRets, throwOnError);
		}
	};

	template <typename U, typename... Us>
	class LuaFunctionBind<U, Us...> {
	public:
		static bool call(LuaState& state, int nArgs, int nRets, bool throwOnError, U u, Us... us)
		{
			ToLua<U>()(state, u);
			return LuaFunctionBind<Us...>::call(state, nArgs + 1, nRets, throwOnError, us...);
		}
	};

	namespace LuaCallbackBindDetails {
		template <typename T>
		struct ArgTransform {
			using type = std::decay_t<T>;
		};

		template <>
		struct ArgTransform<std::string_view> {
			using type = String;
		};

		template <>
		struct ArgTransform<const char*> {
			using type = String;
		};

		template <typename T>
		using ArgTransformT = typename ArgTransform<std::decay_t<T>>::type;

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
		inline std::tuple<ArgTransformT<Ps>...> makeTuple(LuaState& state)
		{
			std::tuple<ArgTransformT<Ps>...> tuple;
			doFillTuple<sizeof...(Ps), std::tuple<ArgTransformT<Ps>...>>(state, tuple);
			return tuple;
		}

		template <typename T, typename R, typename... Ps, typename... As>
		inline R applyTuple(std::enable_if_t<sizeof...(Ps) == sizeof...(As), T*> obj, R (T::*f)(Ps...), std::tuple<ArgTransformT<Ps>...>& tuple, As... args)
		{
			return (obj->*f)(args...);
		}

		template <typename T, typename R, typename... Ps, typename... As>
		inline R applyTuple(std::enable_if_t<sizeof...(Ps) != sizeof...(As), T*> obj, R (T::*f)(Ps...), std::tuple<ArgTransformT<Ps>...>& tuple, As... args)
		{
			return applyTuple(obj, f, tuple, args..., std::get<sizeof...(As)>(tuple));
		}

		template <typename T, typename R, typename... Ps, typename... As>
		inline R applyTuple(std::enable_if_t<sizeof...(Ps) == sizeof...(As), const T*> obj, R (T::*f)(Ps...) const, std::tuple<ArgTransformT<Ps>...>& tuple, As... args)
		{
			return (obj->*f)(args...);
		}

		template <typename T, typename R, typename... Ps, typename... As>
		inline R applyTuple(std::enable_if_t<sizeof...(Ps) != sizeof...(As), const T*> obj, R (T::*f)(Ps...) const, std::tuple<ArgTransformT<Ps>...>& tuple, As... args)
		{
			return applyTuple(obj, f, tuple, args..., std::get<sizeof...(As)>(tuple));
		}

		template <typename T, typename R, typename... Ps>
		inline R call(T* obj, R (T::*f)(Ps...), std::tuple<ArgTransformT<Ps>...>&& args)
		{
			return applyTuple(obj, f, args);
		}

		template <typename T, typename R, typename... Ps>
		inline R call(const T* obj, R (T::*f)(Ps...) const, std::tuple<ArgTransformT<Ps>...>&& args)
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

		template <typename T, typename R, typename... Ps>
		inline LuaCallback bind(const T* obj, R (T::*f)(Ps...) const, std::enable_if_t<std::is_void<R>::value, int>)
		{
			return [=] (LuaState& state) -> int
			{
				call(obj, f, makeTuple<Ps...>(state));
				return 0;
			};
		}

		template <typename T, typename R, typename... Ps>
		inline LuaCallback bind(const T* obj, R (T::*f)(Ps...) const, std::enable_if_t<!std::is_void<R>::value, int>)
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

	template <typename T, typename R, typename... Ps>
	LuaCallback LuaCallbackBind(const T* obj, R (T::*f)(Ps...) const)
	{
		return LuaCallbackBindDetails::bind(obj, f, 0);
	}
}
