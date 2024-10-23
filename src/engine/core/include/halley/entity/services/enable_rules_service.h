#pragma once

#include "halley/entity/service.h"

namespace Halley {
	class ScriptingService;

	class EnableRulesService : public Service {
	public:
		EnableRulesService() = default;

        void clearCache();
        bool evaluateEnableRules(const String& enableRules);

	private:
		ScriptingService* scriptingService = nullptr;
		bool initialized = false;

		void initialize();
	};
}

using EnableRulesService = Halley::EnableRulesService;
