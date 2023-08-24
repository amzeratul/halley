#pragma once
#include "halley/lua/lua_state.h"
#include "halley/scripting/script_environment.h"
#include "halley/entity/service.h"
#include "halley/entity/system_interface.h"

namespace Halley {

	class ScriptingService : public Service, public ILuaInterface {
	public:
		ScriptingService(std::unique_ptr<ScriptEnvironment> environment, Resources& resources, const String& initialLuaModule = "");

		ScriptEnvironment& getEnvironment();

		ConfigNode evaluateExpression(const String& expression) const;
		ConfigNode evaluateExpression(const LuaExpression& expression) const;

		template <typename T>
		void setLuaGlobal(const String& key, const T& value)
		{
			auto stack = LuaStackOps(*luaState);
			stack.push(value);
			stack.makeGlobal(key);
		}

		LuaState& getLuaState() override;

	private:
		std::unique_ptr<ScriptEnvironment> scriptEnvironment;
		std::unique_ptr<LuaState> luaState;
	};
}

using ScriptingService = Halley::ScriptingService;
