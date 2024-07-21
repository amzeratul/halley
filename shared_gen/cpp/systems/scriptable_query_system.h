// Halley codegen version 131
#pragma once

#include <halley.hpp>


#include "components/scriptable_component.h"
#include "components/script_tag_target_component.h"

// Generated file; do not modify.
template <typename T>
class ScriptableQuerySystemBase : private Halley::System {
public:
	class ScriptableFamily : public Halley::FamilyBaseOf<ScriptableFamily> {
	public:
		const ScriptableComponent& scriptable;
	
		using Type = Halley::FamilyType<ScriptableComponent>;
	
	protected:
		ScriptableFamily(const ScriptableComponent& scriptable)
			: scriptable(scriptable)
		{
		}
	};

	class TagTargetsFamily : public Halley::FamilyBaseOf<TagTargetsFamily> {
	public:
		const ScriptTagTargetComponent& scriptTagTarget;
	
		using Type = Halley::FamilyType<ScriptTagTargetComponent>;
	
	protected:
		TagTargetsFamily(const ScriptTagTargetComponent& scriptTagTarget)
			: scriptTagTarget(scriptTagTarget)
		{
		}
	};

	ScriptableQuerySystemBase()
		: System({&scriptableFamily, &tagTargetsFamily}, {})
	{
		static_assert(std::is_final_v<T>, "System must be final.");
	}
protected:
	Halley::World& getWorld() const {
		return doGetWorld();
	}
	Halley::TempMemoryPool& getTempMemoryPool() const {
		return doGetWorld().getTempMemoryPool();
	}
	Halley::FamilyBinding<ScriptableFamily> scriptableFamily{};
	Halley::FamilyBinding<TagTargetsFamily> tagTargetsFamily{};

private:
	friend Halley::System* halleyCreateScriptableQuerySystem();

	void initBase() override final {
		invokeInit<T>(static_cast<T*>(this));
		initialiseFamilyBinding<T, ScriptableFamily>(scriptableFamily, static_cast<T*>(this));
		initialiseFamilyBinding<T, TagTargetsFamily>(tagTargetsFamily, static_cast<T*>(this));
	}

	void updateBase(Halley::Time time) override final {
		static_cast<T*>(this)->update(time);
	}

};

