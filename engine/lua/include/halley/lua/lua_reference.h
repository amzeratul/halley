#pragma once

#include <halley/text/halleystring.h>
#include "lua_function_bind.h"

namespace Halley {
	class LuaState;

	template <typename T>
	class LuaReturnHelper {
	public:
		inline static T cleanUpAndReturn(LuaState& state)
		{
			T result = FromLua<T>()(state);
			LuaFunctionCaller::endCall(state);
			return result;
		}
	};

	template <>
	class LuaReturnHelper<void> {
	public:
		inline static void cleanUpAndReturn(LuaState& state)
		{
			LuaFunctionCaller::endCall(state);
		}
	};

	class LuaReference {
	public:
		LuaReference();
		LuaReference(LuaState& lua);
		LuaReference(LuaReference&& other) noexcept;
		~LuaReference();

		LuaReference& operator=(LuaReference&& other) noexcept;

		LuaReference(const LuaReference& other) = delete;
		LuaReference& operator=(const LuaReference& other) = delete;

		void pushToLuaStack() const;

		LuaReference operator[](const String& name) const;

		template <typename T, typename... Us>
		T call(Us... us) const
		{
			LuaFunctionCaller::startCall(*lua);
			pushToLuaStack();
			LuaFunctionBind<Us...>::call(*lua, 0, LuaReturnSize<T>::value, us...);
			return LuaReturnHelper<T>::cleanUpAndReturn(*lua);
		}

		template <typename T>
		T call() const
		{
			LuaFunctionCaller::startCall(*lua);
			pushToLuaStack();
			LuaFunctionBind<>::call(*lua, 0, LuaReturnSize<T>::value);
			return LuaReturnHelper<T>::cleanUpAndReturn(*lua);
		}

		template <typename T, typename... Us>
		T callMethod(const String& methodName, Us... us) const
		{
			LuaFunctionCaller::startCall(*lua);
			operator[](methodName).pushToLuaStack();
			pushToLuaStack();
			LuaFunctionBind<Us...>::call(*lua, 1, LuaReturnSize<T>::value, us...);
			return LuaReturnHelper<T>::cleanUpAndReturn(*lua);
		}

		template <typename T>
		T callMethod(const String& methodName) const
		{
			LuaFunctionCaller::startCall(*lua);
			operator[](methodName).pushToLuaStack();
			pushToLuaStack();
			LuaFunctionBind<>::call(*lua, 1, LuaReturnSize<T>::value);
			return LuaReturnHelper<T>::cleanUpAndReturn(*lua);
		}

		int getRefId() const { return refId; }

	private:
		LuaState* lua;
		int refId = -1;
	};
	
	template <>
	struct ToLua<const LuaReference&> {
		inline void operator()(LuaState& state, const LuaReference& value) const {
			value.pushToLuaStack();
		}
	};

}
