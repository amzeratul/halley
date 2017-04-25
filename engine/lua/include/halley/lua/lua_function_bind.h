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

	template <typename... Us>
	class LuaFunctionBind;

	template <>
	class LuaFunctionBind<> {
	public:
		static LuaStackReturn call(LuaState& state, LuaReference& ref)
		{
			LuaFunctionCaller::startCall(ref);
			return _doCall(state, 0);
		}
		
		static LuaStackReturn _doCall(LuaState& state, int nArgs)
		{
			LuaFunctionCaller::endCall(state, nArgs, 1);
			return LuaStackReturn(state);
		}
	};

	template <typename U, typename... Us>
	class LuaFunctionBind<U, Us...> {
	public:
		static LuaStackReturn call(LuaState& state, LuaReference& ref, U u, Us... us)
		{
			LuaFunctionCaller::startCall(ref);
			return _doCall(state, 0, u, us...);
		}

		static LuaStackReturn _doCall(LuaState& state, int nArgs, U u, Us... us)
		{
			LuaFunctionCaller::push(state, u);
			return LuaFunctionBind<Us...>::_doCall(state, nArgs + 1, us...);
		}
	};
}
