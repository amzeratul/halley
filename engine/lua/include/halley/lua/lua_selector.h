#pragma once

namespace Halley {
	class LuaReference;
	class LuaState;

	class LuaSelector {
	public:
		LuaSelector(LuaState& state);
		LuaSelector(LuaState& state, const LuaReference& ref);
		
		LuaSelector operator[](const String& name);

	private:
		LuaState& state;
		int ref;
	};
}
