#include <lua.hpp>
#include "lua_state.h"
#include "halley/support/exception.h"
#include "halley/support/logger.h"
#include "halley/core/resources/resources.h"
#include "halley/file_formats/binary_file.h"

using namespace Halley;

LuaState::LuaState(Resources& resources)
	: lua(luaL_newstate())
	, resources(&resources)
{
	luaL_openlibs(lua);
			
	// TODO: convert this into an automatic table
	LuaStackUtils u(*this);
	u.pushTable();
	u.setField("print", LuaCallbackBind(this, &LuaState::print));
	u.setField("errorHandler", LuaCallbackBind(this, &LuaState::errorHandler));
	u.setField("packageLoader", LuaCallbackBind(this, &LuaState::packageLoader));
	u.makeGlobal("halleyAPI");

	lua_getglobal(lua, "halleyAPI");
	lua_getfield(lua, -1, "errorHandler");
	errorHandlerRef = std::make_unique<LuaReference>(*this);
	lua_pop(lua, 1);

	auto res = resources.get<BinaryFile>("lua/halley/halley.lua");
	loadModule("halley", res->getSpan());
}

LuaState::~LuaState()
{
	modules.clear();
	closures.clear();
	errorHandlerRef.reset();
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

const LuaReference& LuaState::getOrLoadModule(const String& moduleName)
{
	auto result = tryGetModule(moduleName);
	if (!result) {
		auto res = resources->get<BinaryFile>("lua/" + moduleName + ".lua");
		return loadModule(moduleName, res->getSpan());
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
	int result = lua_pcall(lua, nArgs, nRets, errorHandlerStackPos.empty() ? 0 : errorHandlerStackPos.back());

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

void LuaState::pushErrorHandler()
{
	if (errorHandlerRef) {
		errorHandlerRef->pushToLuaStack();
		errorHandlerStackPos.push_back(lua_gettop(lua));
	}
}

void LuaState::popErrorHandler()
{
	if (errorHandlerRef) {
		if (errorHandlerStackPos.empty()) {
			throw Exception("Error handler not set.");
		}
		if (errorHandlerStackPos.back() != lua_gettop(lua)) {
			throw Exception("Stack corruption.");
		}
		lua_pop(lua, 1);
		errorHandlerStackPos.pop_back();
	}
}

String LuaState::errorHandler(String message)
{
	String result = message;
	
	lua_Debug stack;
	for (int i = 1; lua_getstack(lua, i, &stack); ++i) {
		lua_getinfo(lua, "nSlf", &stack);
		
		String name;
		if (stack.namewhat && String(stack.namewhat) != "") {
			name = stack.namewhat + String(" ");
		}
		if (stack.name) {
			name += stack.name;
		} else {
			name += "<unknown>";
		}

		String args;
		{
			const char* argName = nullptr;
			for (int j = 1; (argName = lua_getlocal(lua, &stack, j)) != nullptr && j <= stack.nparams; ++j) {
				if (j > 1) {
					args += ", ";
				}
				args += String(argName) + " = " + printVariableAtTop();
			}
		}
		
		result += "\n\t" + toString(i) + ": " + name + " (" + args + ") [" + stack.source + ":" + toString(stack.currentline) + "]";

		{
			const char* localName = nullptr;
			for (int j = 1; (localName = lua_getupvalue(lua, -1, j)) != nullptr; ++j) {
				result += "\n\t\tU " + String(localName) + " = " + printVariableAtTop();
			}
		}
		{
			const char* localName = nullptr;
			for (int j = 1 + stack.nparams; (localName = lua_getlocal(lua, &stack, j)) != nullptr; ++j) {
				result += "\n\t\tL " + String(localName) + " = " + printVariableAtTop();
			}
		}
	}

	return result;
}

const LuaReference& LuaState::packageLoader(String module)
{
	return getOrLoadModule(module);
}

String LuaState::printVariableAtTop()
{
	String result;

	if (lua_istable(lua, -1)) {
		result = toString("{...}");
	} else if (lua_isinteger(lua, -1)) {
		result = toString(lua_tointeger(lua, -1));
	} else if (lua_isstring(lua, -1)) {
		result = String("\"") + lua_tostring(lua, -1) + "\"";
	} else if (lua_isnumber(lua, -1)) {
		result = toString(lua_tonumber(lua, -1));
	} else if (lua_isnoneornil(lua, -1)) {
		result = "nil";
	} else if (lua_isfunction(lua, -1)) {
		result = "function";
	} else if (lua_isboolean(lua, -1)) {
		result = lua_toboolean(lua, -1) != 0? "true" : "false";
	} else if (lua_isthread(lua, -1)) {
		result = "thread";
	}

	lua_pop(lua, 1);
	return result;
}
