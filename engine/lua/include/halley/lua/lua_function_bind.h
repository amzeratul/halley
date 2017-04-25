#pragma once
#include "halley/maths/vector2.h"
#include "halley/data_structures/maybe.h"
#include <cstdint>
#include <cstddef>

namespace Halley {
	class String;
	class LuaState;
	class LuaReference;

	class LuaFunctionCaller {
	public:
		static void startCall(LuaReference& ref);
		static void endCall(LuaState& state, int nArgs, int nRets);

		static void push(LuaState& state, std::nullptr_t n);
		static void push(LuaState& state, bool v);
		static void push(LuaState& state, int v);
		static void push(LuaState& state, int64_t v);
		static void push(LuaState& state, double v);
		static void push(LuaState& state, const String& v);
		static void push(LuaState& state, Vector2i v);

		template <typename T>
		static void push(LuaState& state, Maybe<T> v)
		{
			if (v) {
				push(state, v.get());
			} else {
				push(state, nullptr);
			}
		}
	};

	template <typename T>
	struct LuaReturnSize {
		static constexpr int value = 1;
	};

	template <>
	struct LuaReturnSize<void> {
		static constexpr int value = 0;
	};

	class LuaStackReturn {
	public:
		explicit LuaStackReturn(LuaState& state);

		operator bool() const;
		operator int() const;
		operator int64_t() const;
		operator double() const;
		operator String() const;
		operator Vector2i() const;

	private:
		LuaState& state;
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
			LuaFunctionCaller::push(state, u);
			LuaFunctionBind<Us...>::_doCall(state, nArgs + 1, nRets, us...);
		}
	};
}
