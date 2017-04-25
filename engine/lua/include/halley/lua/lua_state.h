#pragma once

#include <gsl/gsl>
#include <halley/text/halleystring.h>
#include "lua_reference.h"
#include <unordered_map>

struct lua_State;

namespace Halley {
	class LuaState;

	class LuaState {
	public:
		LuaState();
		~LuaState();

		const LuaReference* tryGetModule(const String& moduleName) const;
		const LuaReference& getModule(const String& moduleName) const;
		const LuaReference& loadModule(const String& moduleName, gsl::span<const gsl::byte> data);
		void unloadModule(const String& moduleName);

		lua_State* getRawState();

		template <typename T, typename F>
		void pushCallback(T& obj, F function)
		{
			doPushCallback(LuaCallbackBind::bindCallback(&obj, function));
		}

	private:
		lua_State* lua;
		std::unordered_map<String, LuaReference> modules;
		std::vector<std::unique_ptr<LuaCallbackBind::LuaCallback>> closures;

		LuaReference loadScript(const String& chunkName, gsl::span<const gsl::byte> data);
		void doPushCallback(LuaCallbackBind::LuaCallback&& callback);
	};
}
