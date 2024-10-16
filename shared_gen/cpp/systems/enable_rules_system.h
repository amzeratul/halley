// Halley codegen version 136
#pragma once

#include <halley.hpp>

#include "halley/entity/services/scripting_service.h"


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

	ScriptingService& getScriptingService() const {
		return *scriptingService;
	}

private:
	friend Halley::System* halleyCreateEnableRulesSystem();

	ScriptingService* scriptingService{ nullptr };
	void initBase() override final {
		scriptingService = &doGetWorld().template getService<ScriptingService>(getName());
		invokeInit<T>(static_cast<T*>(this));
	}

	void updateBase(Halley::Time time) override final {
		static_cast<T*>(this)->update(time);
	}

};

