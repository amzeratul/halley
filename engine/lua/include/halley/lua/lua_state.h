#pragma once

#include <memory>
#include <gsl/gsl>
#include <halley/text/halleystring.h>

struct lua_State;

namespace Halley {
	class LuaState {
	public:
		LuaState();
		~LuaState();

		void loadScript(const String& chunkName, gsl::span<const gsl::byte> data);

	private:
		lua_State* lua;
	};
}
