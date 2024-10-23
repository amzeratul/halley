#include "halley/entity/services/enable_rules_service.h"
#include "halley/entity/services/scripting_service.h"

using namespace Halley;

void EnableRulesService::clearCache()
{
	if (scriptingService) {
		scriptingService->clearResultCache();
	}
}

bool EnableRulesService::evaluateEnableRules(const String& enableRules)
{
	// Ensure scripting service is loaded
	initialize();
	if (!scriptingService) {
		return true;
	}

	return scriptingService->evaluateExpression(enableRules, true).asBool(true);
}

void EnableRulesService::initialize()
{
	if (!initialized) {
		scriptingService = getWorld().tryGetService<ScriptingService>();
		initialized = true;
	}
}
