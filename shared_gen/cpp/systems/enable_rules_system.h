// Halley codegen version 136
#pragma once

#include <halley.hpp>

#include "halley/entity/services/enable_rules_service.h"


// Generated file; do not modify.
template <typename T>
class EnableRulesSystemBase : private Halley::System {
public:
	EnableRulesSystemBase()
		: System({}, {})
	{
		static_assert(std::is_final_v<T>, "System must be final.");
	}
protected:
	Halley::World& getWorld() const {
		return doGetWorld();
	}
	Halley::TempMemoryPool& getTempMemoryPool() const {
		return doGetWorld().getUpdateMemoryPool();
	}

	EnableRulesService& getEnableRulesService() const {
		return *enableRulesService;
	}

private:
	friend Halley::System* halleyCreateEnableRulesSystem();

	EnableRulesService* enableRulesService{ nullptr };
	void initBase() override final {
		enableRulesService = &doGetWorld().template getService<EnableRulesService>(getName());
		invokeInit<T>(static_cast<T*>(this));
	}

	void updateBase(Halley::Time time) override final {
		static_cast<T*>(this)->update(time);
	}

};

