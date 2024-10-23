#pragma once
#include "halley/lua/lua_state.h"
#include "halley/scripting/script_environment.h"
#include "halley/entity/service.h"
#include "halley/entity/system_interface.h"

namespace Halley {

	class ScriptingService : public Service, public ILuaInterface {
	public:
		ScriptingService(std::unique_ptr<ScriptEnvironment> environment, Resources& resources, const String& initialLuaModule = "");

		ScriptEnvironment& getEnvironment() const;

		ConfigNode evaluateExpression(const String& expression, bool useResultCache = false) const;
		ConfigNode evaluateExpression(const LuaExpression& expression, bool useResultCache = false) const;
		void clearResultCache();

		template <typename T>
		void setLuaGlobal(const String& key, const T& value)
		{
			auto stack = LuaStackOps(*luaState);
			stack.push(value);
			stack.makeGlobal(key);

			globals[key] = ConfigNode(value);
		}

		ConfigNode getLuaGlobal(const String& key);
		void copyLuaGlobal(const String& key, ScriptingService& source);
		LuaState& getLuaState() override;

		std::shared_ptr<ScriptingService> clone(std::unique_ptr<ScriptEnvironment> environment = {}) const;

	private:
		std::unique_ptr<ScriptEnvironment> scriptEnvironment;
		std::unique_ptr<LuaState> luaState;
		HashMap<String, ConfigNode> globals;
		String initialModule;
		Resources& resources;

	    mutable HashMap<String, ConfigNode> resultCache;
	};
}

using ScriptingService = Halley::ScriptingService;
