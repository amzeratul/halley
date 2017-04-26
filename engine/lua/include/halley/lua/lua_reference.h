#pragma once

#include <halley/text/halleystring.h>
#include "lua_function_bind.h"

namespace Halley {
	class LuaState;

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
		T call(Us... us)
		{
			pushToLuaStack();
			LuaFunctionBind<Us...>::call(*lua, LuaReturnSize<T>::value, us...);
			return LuaConvert<T>::fromStack(*lua);
		}

		template <typename T>
		T call()
		{
			pushToLuaStack();
			LuaFunctionBind<>::call(*lua, LuaReturnSize<T>::value);
			return LuaConvert<T>::fromStack(*lua);
		}

	private:
		LuaState* lua;
		int refId = -1;
	};
}
