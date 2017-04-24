#include "lua_state.h"
#include "lua.hpp"
#include "halley/support/exception.h"

using namespace Halley;

LuaReference::LuaReference()
	: lua(nullptr)
	, refId(LUA_NOREF)
{
}

LuaReference::LuaReference(LuaState& l)
	: lua(&l)
{
	refId = luaL_ref(lua->getRawState(), LUA_REGISTRYINDEX);
}

LuaReference::LuaReference(LuaReference&& other) noexcept
{
	lua = other.lua;
	refId = other.refId;
	other.refId = LUA_NOREF;
}

LuaReference& LuaReference::operator=(LuaReference&& other) noexcept
{
	lua = other.lua;
	refId = other.refId;
	other.refId = LUA_NOREF;
	return *this;
}

LuaReference::~LuaReference()
{
	if (refId != LUA_NOREF && lua!= nullptr) {
		luaL_unref(lua->getRawState(), LUA_REGISTRYINDEX, refId);
	}
}

void LuaReference::pushToLuaStack() const
{
	Expects (refId != LUA_NOREF);
	Expects (lua);

	lua_rawgeti(lua->getRawState(), LUA_REGISTRYINDEX, refId);
}

LuaReference LuaReference::operator[](const String& name) const
{
	pushToLuaStack();
	lua_getfield(lua->getRawState(), -1, name.c_str());
	if (lua_isnil(lua->getRawState(), -1)) {
		throw Exception("Unknown field: " + name);
	}
	return LuaReference(*lua);
}

void LuaReference::operator()() const
{
	pushToLuaStack();
	lua_pcall(lua->getRawState(), 0, 0, 0);
}

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
		throw Exception("Error running lua chunk.");
	}

	// Store chunk in registry
	return LuaReference(*this);
}
