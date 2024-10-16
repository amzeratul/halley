#pragma once

#include "halley/entity/service.h"
#include "halley/lua/lua_reference.h"

namespace Halley {
	class ScriptingService;

	class EnableRulesService : public Service {
	public:
		EnableRulesService() = default;

        void resetCache();
        bool evaluateEnableRules(const String& enableRules);

	private:
		ScriptingService* scriptingService = nullptr;

    	HashMap<String, LuaExpression> expressionCache;
	    HashMap<String, bool> resultCache;
	};
}

using EnableRulesService = Halley::EnableRulesService;
