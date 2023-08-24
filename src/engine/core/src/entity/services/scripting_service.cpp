#include "halley/entity/services/scripting_service.h"

using namespace Halley;

ScriptingService::ScriptingService(std::unique_ptr<ScriptEnvironment> env, Resources& resources, const String& initialModule)
{
	scriptEnvironment = std::move(env);
	scriptEnvironment->getWorld().setInterface(static_cast<ILuaInterface*>(this));

	luaState = std::make_unique<LuaState>(resources);
	if (!initialModule.isEmpty()) {
		luaState->getOrLoadModule(initialModule);
	}
}

ScriptEnvironment& ScriptingService::getEnvironment()
{
	return *scriptEnvironment;
}

ConfigNode ScriptingService::evaluateExpression(const String& expression) const
{
	auto stack = LuaStackOps(*luaState);
	stack.eval("return " + expression);
	return stack.popConfigNode();
}

ConfigNode ScriptingService::evaluateExpression(const LuaExpression& expression) const
{
	if (expression.isEmpty()) {
		return ConfigNode();
	}
	return expression.get(*luaState).call<ConfigNode>();
}

LuaState& ScriptingService::getLuaState()
{
	return *luaState;
}
