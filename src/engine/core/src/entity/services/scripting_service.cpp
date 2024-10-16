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

ScriptEnvironment& Halley::ScriptingService::getEnvironment() const
{
	return *scriptEnvironment;
}

ConfigNode Halley::ScriptingService::evaluateExpression(const String& expression) const
{
	auto stack = LuaStackOps(*luaState);
	stack.eval("return " + expression);
	return stack.popConfigNode();
}

ConfigNode Halley::ScriptingService::evaluateExpression(const LuaExpression& expression) const
{
	if (expression.isEmpty()) {
		return ConfigNode();
	}
	return expression.get(*luaState).call<ConfigNode>();
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

LuaState& Halley::ScriptingService::getLuaState()
{
	return *luaState;
}

std::shared_ptr<ScriptingService> ScriptingService::clone(std::unique_ptr<ScriptEnvironment> environment) const
{
	auto result = std::make_shared<ScriptingService>(std::move(environment), resources, initialModule);
	for (const auto& [key, value]: globals) {
		result->setLuaGlobal(key, value);
	}
	return result;
}
