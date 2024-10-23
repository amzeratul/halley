#include "halley/entity/services/scripting_service.h"

using namespace Halley;

ScriptingService::ScriptingService(std::unique_ptr<ScriptEnvironment> env, Resources& resources, const String& initialModule)
	: initialModule(initialModule)
	, resources(resources)
{
	scriptEnvironment = std::move(env);
	if (scriptEnvironment) {
		scriptEnvironment->getWorld().setInterface(static_cast<ILuaInterface*>(this));
	}

	luaState = std::make_unique<LuaState>(resources);
	if (!initialModule.isEmpty()) {
		luaState->getOrLoadModule(initialModule);
	}
}

ScriptEnvironment& ScriptingService::getEnvironment() const
{
	return *scriptEnvironment;
}

ConfigNode ScriptingService::evaluateExpression(const String& expression, bool useResultCache) const
{
	if (useResultCache) {
		if (auto iter = resultCache.find(expression); iter != resultCache.end()) {
			return ConfigNode(iter->second);
		}
	}

	auto stack = LuaStackOps(*luaState);
	stack.eval("return " + expression);
	auto result = stack.popConfigNode();

	if (useResultCache) {
		resultCache[expression] = ConfigNode(result);
	}
	return result;
}

ConfigNode ScriptingService::evaluateExpression(const LuaExpression& expression, bool useResultCache) const
{
	if (expression.isEmpty()) {
		return ConfigNode();
	}

	if (useResultCache) {
		if (auto iter = resultCache.find(expression.getExpression()); iter != resultCache.end()) {
			return ConfigNode(iter->second);
		}
	}

	auto result = expression.get(*luaState).call<ConfigNode>();

	if (useResultCache) {
		resultCache[expression.getExpression()] = ConfigNode(result);
	}
	return result;
}

void ScriptingService::clearResultCache()
{
	resultCache.clear();
}

ConfigNode ScriptingService::getLuaGlobal(const String& key)
{
	auto stack = LuaStackOps(*luaState);
	stack.getGlobal(key);
	return stack.popConfigNode();
}

void ScriptingService::copyLuaGlobal(const String& key, ScriptingService& source)
{
	auto stack = LuaStackOps(*luaState);
	stack.push(source.getLuaGlobal(key));
	stack.makeGlobal(key);
}

LuaState& ScriptingService::getLuaState()
{
	return *luaState;
}

std::shared_ptr<ScriptingService> ScriptingService::clone(std::unique_ptr<ScriptEnvironment> environment) const
{
	auto result = std::make_shared<ScriptingService>(std::move(environment), resources, initialModule);
	for (const auto& [key, value]: globals) {
		result->setLuaGlobal(key, value);
	}
	result->resultCache = resultCache;
	return result;
}
