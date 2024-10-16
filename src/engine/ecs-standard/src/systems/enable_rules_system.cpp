#include <systems/enable_rules_system.h>

using namespace Halley;

class EnableRulesSystem final : public EnableRulesSystemBase<EnableRulesSystem>, IEnableRulesSystemInterface {
public:
	void init()
	{
		getWorld().setInterface<IEnableRulesSystemInterface>(this);
        refreshEnabled();
	}

	void update(Time t)
	{
	}

    void refreshEnabled() override
    {
		resultCache.clear();

		auto& world = getWorld();
		for (auto* e: world.getRawEntities()) {
			refreshEntity(EntityRef(*e, world));
		}
    }

private:

	HashMap<String, LuaExpression> expressionCache;
	HashMap<String, bool> resultCache;

	void refreshEntity(EntityRef e)
	{
		const auto& enableRules = e.getEnableRules();
		if (!enableRules.isEmpty()) {
			e.setEnabled(evaluate(enableRules));
		}
	}

	bool evaluate(const String& enableRules)
	{
		// See if it's cached
		if (auto iter = resultCache.find(enableRules); iter != resultCache.end()) {
			return iter->second;
		}

		// Find the expression in the cache, or create one if needed
		auto iter = expressionCache.find(enableRules);
		if (iter == expressionCache.end()) {
			expressionCache[enableRules] = LuaExpression(enableRules);
			iter = expressionCache.find(enableRules);
		}

		// Evaluate, cache, and return
		bool result = getScriptingService().evaluateExpression(iter->second).asBool(true);
		resultCache[enableRules] = result;
		return result;
	}
};

REGISTER_SYSTEM(EnableRulesSystem)

