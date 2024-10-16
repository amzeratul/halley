#include "halley/entity/services/enable_rules_service.h"
#include "halley/entity/services/scripting_service.h"

using namespace Halley;

void EnableRulesService::resetCache()
{
	resultCache.clear();
}

bool EnableRulesService::evaluateEnableRules(const String& enableRules)
{
	// Ensure scripting service is loaded
	initialize();
	if (!scriptingService) {
		return true;
	}

	// See if it's cached
	if (auto iter = resultCache.find(enableRules); iter != resultCache.end()) {
		//Logger::logDev(enableRules + " = " + toString(iter->second) + " (cached)");
		return iter->second;
	}

	// Find the expression in the cache, or create one if needed
	auto iter = expressionCache.find(enableRules);
	if (iter == expressionCache.end()) {
		expressionCache[enableRules] = LuaExpression(enableRules);
		iter = expressionCache.find(enableRules);
	}

	// Evaluate, cache, and return
	bool result = scriptingService->evaluateExpression(iter->second).asBool(true);
	resultCache[enableRules] = result;
	//Logger::logDev(enableRules + " = " + toString(result));
	return result;
}

void EnableRulesService::initialize()
{
	if (!initialized) {
		scriptingService = getWorld().tryGetService<ScriptingService>();
		initialized = true;
	}
}
