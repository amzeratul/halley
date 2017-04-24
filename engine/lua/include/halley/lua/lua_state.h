#pragma once

#include <memory>
#include <gsl/gsl>
#include <halley/text/halleystring.h>
#include "lua_selector.h"
#include <unordered_map>

struct lua_State;

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

		void operator()() const;

	private:
		LuaState* lua;
		int refId = -1;
	};

	class LuaState {
	public:
		LuaState();
		~LuaState();

		const LuaReference* tryGetModule(const String& moduleName) const;
		const LuaReference& getModule(const String& moduleName) const;
		const LuaReference& loadModule(const String& moduleName, gsl::span<const gsl::byte> data);
		void unloadModule(const String& moduleName);

		lua_State* getRawState();

	private:
		lua_State* lua;
		std::unordered_map<String, LuaReference> modules;

		LuaReference loadScript(const String& chunkName, gsl::span<const gsl::byte> data);
	};
}
