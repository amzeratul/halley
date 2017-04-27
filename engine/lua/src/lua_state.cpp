#include <lua.hpp>
#include "lua_state.h"
#include "halley/support/exception.h"
#include "halley/support/logger.h"

using namespace Halley;

LuaState::LuaState()
	: lua(luaL_newstate())
{
	luaL_openlibs(lua);

	// TODO: convert this into an automatic table
	LuaStackUtils u(*this);
	u.push(LuaTable());
	u.setField("print", LuaCallbackBind(this, &LuaState::print));
	u.makeGlobal("halleyAPI");
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

void LuaState::call(int nArgs, int nRets)
{
	int result = lua_pcall(lua, nArgs, nRets, 0);
	if (result != 0) {
		throw Exception("Lua exception:\n\t" + LuaStackOps(*this).popString());
	}
}

lua_State* LuaState::getRawState()
{
	return lua;
}

LuaReference LuaState::loadScript(const String& chunkName, gsl::span<const gsl::byte> data)
{
	int result = luaL_loadbuffer(lua, reinterpret_cast<const char*>(data.data()), data.size_bytes(), chunkName.c_str());
	if (result != 0) {
		throw Exception("Error loading Lua chunk:\n\t" + LuaStackOps(*this).popString());
	}
	call(0, 1);

	// Store chunk in registry
	auto ref = LuaReference(*this);
	return std::move(ref);
}

void LuaState::print(String string)
{
	Logger::logInfo(string);
}

static int luaClosureInvoker(lua_State* lua)
{
	LuaCallback* callback = reinterpret_cast<LuaCallback*>(lua_touserdata(lua, lua_upvalueindex(1)));
	LuaState* state = reinterpret_cast<LuaState*>(lua_touserdata(lua, lua_upvalueindex(2)));
	return (*callback)(*state);
}

void LuaState::pushCallback(LuaCallback&& callback)
{
	closures.push_back(std::make_unique<LuaCallback>(callback));

	lua_pushlightuserdata(lua, closures.back().get());
	lua_pushlightuserdata(lua, this);
	lua_pushcclosure(lua, luaClosureInvoker, 2);
}
