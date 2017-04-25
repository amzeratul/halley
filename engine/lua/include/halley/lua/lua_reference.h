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

		template <typename... Us>
		LuaStackReturn operator()(Us... us)
		{
			return LuaFunctionBind<Us...>::call(*lua, *this, us...);
		}

		LuaStackReturn operator()()
		{
			return LuaFunctionBind<>::call(*lua, *this);
		}

	private:
		LuaState* lua;
		int refId = -1;
	};
}
