#include <lua.hpp>
#include "lua_state.h"
#include "halley/support/exception.h"

using namespace Halley;

LuaState::LuaState()
	: lua(luaL_newstate())
{
	luaL_openlibs(lua);
}

LuaState::~LuaState()
{
	modules.clear();
	lua_close(lua);
}

const LuaReference* LuaState::tryGetModule(const String& moduleName) const
{
	auto iter = modules.find(moduleName);
	if (iter == modules.end()) {
		return nullptr;
	}
	return &iter->second;
}

const LuaReference& LuaState::getModule(const String& moduleName) const
{
	auto result = tryGetModule(moduleName);
	if (!result) {
		throw Exception("Module not found: " + moduleName);
	}
	return *result;
}

const LuaReference& LuaState::loadModule(const String& moduleName, gsl::span<const gsl::byte> data)
{
	modules[moduleName] = loadScript(moduleName, data);
	return getModule(moduleName);
}

void LuaState::unloadModule(const String& moduleName)
{
	auto iter = modules.find(moduleName);
	if (iter == modules.end()) {
		throw Exception("Module not loaded: " + moduleName);
	}
	modules.erase(iter);
}

lua_State* LuaState::getRawState()
{
	return lua;
}

LuaReference LuaState::loadScript(const String& chunkName, gsl::span<const gsl::byte> data)
{
	int result = luaL_loadbuffer(lua, reinterpret_cast<const char*>(data.data()), data.size_bytes(), chunkName.c_str());
	if (result != 0) {
		throw Exception("Error loading Lua chunk, add error checking here.");
	}

	// Run chunk
	result = lua_pcall(lua, 0, 1, 0);
	if (result != 0) {
		throw Exception("Error running lua chunk: " + LuaStackOps(*this).popString());
	}

	// Store chunk in registry
	auto ref = LuaReference(*this);
	return std::move(ref);
}
