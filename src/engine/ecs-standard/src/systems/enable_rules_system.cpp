#include <systems/enable_rules_system.h>

using namespace Halley;

class EnableRulesSystem final : public EnableRulesSystemBase<EnableRulesSystem>, IEnableRulesSystemInterface {
public:
	void init()
	{
		getWorld().setInterface<IEnableRulesSystemInterface>(this);
	}

	void update(Time t)
	{
	}

    void refreshEnabled() override
    {
		getEnableRulesService().clearCache();

		auto& world = getWorld();
		for (auto* e: world.getRawEntities()) {
			refreshEntity(EntityRef(*e, world));
		}
    }
	
	void refreshEntity(EntityRef e)
	{
		const auto& enableRules = e.getEnableRules();
		if (!enableRules.isEmpty()) {
			e.setEnabled(getEnableRulesService().evaluateEnableRules(enableRules));
		}
	}
};

REGISTER_SYSTEM(EnableRulesSystem)

