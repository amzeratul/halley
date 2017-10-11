#pragma once

#include <gsl/gsl>
#include <halley/text/halleystring.h>
#include "lua_reference.h"
#include <unordered_map>

struct lua_State;

namespace Halley {
	class Resources;
	class LuaState;

	class LuaState {
	public:
		LuaState(Resources& resources);
		~LuaState();

		const LuaReference* tryGetModule(const String& moduleName) const;
		const LuaReference& getModule(const String& moduleName) const;
		const LuaReference& getOrLoadModule(const String& moduleName);
		const LuaReference& loadModule(const String& moduleName, gsl::span<const gsl::byte> data);
		void unloadModule(const String& moduleName);

		void call(int nArgs, int nRets);

		lua_State* getRawState();
		
		void pushCallback(LuaCallback&& callback);

		void pushErrorHandler();
		void popErrorHandler();
		void pushLuaState(lua_State* lua);
		void popLuaState();
		String errorHandler(String message);

	private:
		lua_State* lua;
		std::vector<lua_State*> pushedStates;
		Resources* resources;

		std::unordered_map<String, LuaReference> modules;
		std::vector<std::unique_ptr<LuaCallback>> closures;
		std::unique_ptr<LuaReference> errorHandlerRef;
		std::vector<int> errorHandlerStackPos;

		LuaReference loadScript(const String& chunkName, gsl::span<const gsl::byte> data);

		void print(String string);
		const LuaReference& packageLoader(String moduleName);
		String printVariableAtTop(int maxDepth = 2, bool quote = true);
	};

	class LuaStateOverrider {
	public:
		LuaStateOverrider(LuaState& state, lua_State* rawState);
		~LuaStateOverrider();

	private:
		LuaState& state;
	};
}
