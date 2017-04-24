#include "lua_state.h"
#include "lua.hpp"
#include "halley/support/exception.h"

Halley::LuaState::LuaState()
	: lua(luaL_newstate())
{
	luaL_openlibs(lua);
}

Halley::LuaState::~LuaState()
{
	lua_close(lua);
}

void Halley::LuaState::loadScript(const String& chunkName, gsl::span<const gsl::byte> data)
{
	int result = luaL_loadbuffer(lua, reinterpret_cast<const char*>(data.data()), data.size_bytes(), chunkName.c_str());
	if (result != 0) {
		throw Exception("Error loading Lua chunk.");
	}
}
