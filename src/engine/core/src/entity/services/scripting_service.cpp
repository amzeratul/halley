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

ScriptingService::~ScriptingService()
{
	luaReferences.clear();
	luaState = {};
}

ScriptEnvironment& ScriptingService::getEnvironment() const
{
	return *scriptEnvironment;
}

ConfigNode ScriptingService::evaluateExpression(const String& expression, bool useResultCache, bool throwOnError) const
{
	if (useResultCache) {
		if (auto iter = resultCache.find(expression); iter != resultCache.end()) {
			return ConfigNode(iter->second);
		}
	}

	auto stack = LuaStackOps(*luaState);
	const bool ok = stack.eval("return " + expression, "", throwOnError);
	auto result = stack.popConfigNode();

	if (!ok) {
		String errorMsg = "Error evaluating Lua expression: \"" + expression + "\"";
		if (throwOnError) {
			throw Exception(errorMsg, HalleyExceptions::Scripting);
		} else {
			Logger::logError(errorMsg);
		}
	}

	if (useResultCache) {
		resultCache[expression] = ConfigNode(result);
	}
	return result;
}

ConfigNode ScriptingService::evaluateExpression(const LuaExpression& expression, bool useResultCache, bool throwOnError) const
{
	if (expression.isEmpty()) {
		return ConfigNode();
	}

	if (useResultCache) {
		if (auto iter = resultCache.find(expression.getExpression()); iter != resultCache.end()) {
			return ConfigNode(iter->second);
		}
	}

	auto& expr = getLuaReference(expression);

	ConfigNode result;

	if (throwOnError) {
		try {
			result = expr.call<ConfigNode>();
		} catch (...) {
			Logger::logError("Error while executing Lua expression \"" + expression.getExpression() + "\"");
			throw;
		}
	} else {
		if (auto v = expr.callNoThrow<ConfigNode>()) {
			result = std::move(*v);
		} else {
			Logger::logError("Error while executing Lua expression \"" + expression.getExpression() + "\"");
		}
	}

	if (useResultCache) {
		resultCache[expression.getExpression()] = ConfigNode(result);
	}
	return result;
}

bool ScriptingService::evaluateBoolExpression(const String& expression, bool onEmpty, bool useResultCache, bool throwOnError) const
{
	if (expression.isEmpty()) {
		return onEmpty;
	}
	return evaluateExpression(expression, useResultCache, throwOnError).asBool(false);
}

bool ScriptingService::evaluateBoolExpression(const LuaExpression& expression, bool onEmpty, bool useResultCache, bool throwOnError) const
{
	if (expression.isEmpty()) {
		return onEmpty;
	}
	return evaluateExpression(expression, useResultCache, throwOnError).asBool(false);
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

LuaState& ScriptingService::getLuaState() const
{
	return *luaState;
}

LuaReference& ScriptingService::getLuaReference(const LuaExpression& luaExpression) const
{
	const auto& key = luaExpression.getExpression();

	const auto iter = luaReferences.find(key);
	if (iter != luaReferences.end()) {
		return *iter->second;
	}
	auto [iter2, inserted] = luaReferences.insert_or_assign(key, luaExpression.makeReference(*luaState));
	return *iter2->second;
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
