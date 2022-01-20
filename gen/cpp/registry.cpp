#include <halley.hpp>
using namespace Halley;

#include "halley/entity/components/transform_2d_component.h"
#include "components/sprite_component.h"
#include "components/text_label_component.h"
#include "components/sprite_animation_component.h"
#include "components/camera_component.h"
#include "components/particles_component.h"
#include "components/audio_listener_component.h"
#include "components/audio_source_component.h"
#include "components/script_component.h"
#include "components/script_target_component.h"
#include "components/network_component.h"

// System factory functions


using SystemFactoryPtr = System* (*)();
using SystemFactoryMap = HashMap<String, SystemFactoryPtr>;

static SystemFactoryMap makeSystemFactories() {
	SystemFactoryMap result;
	return result;
}


using ComponentFactoryPtr = std::function<CreateComponentFunctionResult(const EntityFactoryContext&, EntityRef&, const ConfigNode&)>;
using ComponentFactoryMap = HashMap<String, ComponentFactoryPtr>;

static ComponentFactoryMap makeComponentFactories() {
	ComponentFactoryMap result;
	result["Transform2D"] = [] (const EntityFactoryContext& context, EntityRef& e, const ConfigNode& node) -> CreateComponentFunctionResult { return context.createComponent<Transform2DComponent>(e, node); };
	result["Sprite"] = [] (const EntityFactoryContext& context, EntityRef& e, const ConfigNode& node) -> CreateComponentFunctionResult { return context.createComponent<SpriteComponent>(e, node); };
	result["TextLabel"] = [] (const EntityFactoryContext& context, EntityRef& e, const ConfigNode& node) -> CreateComponentFunctionResult { return context.createComponent<TextLabelComponent>(e, node); };
	result["SpriteAnimation"] = [] (const EntityFactoryContext& context, EntityRef& e, const ConfigNode& node) -> CreateComponentFunctionResult { return context.createComponent<SpriteAnimationComponent>(e, node); };
	result["Camera"] = [] (const EntityFactoryContext& context, EntityRef& e, const ConfigNode& node) -> CreateComponentFunctionResult { return context.createComponent<CameraComponent>(e, node); };
	result["Particles"] = [] (const EntityFactoryContext& context, EntityRef& e, const ConfigNode& node) -> CreateComponentFunctionResult { return context.createComponent<ParticlesComponent>(e, node); };
	result["AudioListener"] = [] (const EntityFactoryContext& context, EntityRef& e, const ConfigNode& node) -> CreateComponentFunctionResult { return context.createComponent<AudioListenerComponent>(e, node); };
	result["AudioSource"] = [] (const EntityFactoryContext& context, EntityRef& e, const ConfigNode& node) -> CreateComponentFunctionResult { return context.createComponent<AudioSourceComponent>(e, node); };
	result["Script"] = [] (const EntityFactoryContext& context, EntityRef& e, const ConfigNode& node) -> CreateComponentFunctionResult { return context.createComponent<ScriptComponent>(e, node); };
	result["ScriptTarget"] = [] (const EntityFactoryContext& context, EntityRef& e, const ConfigNode& node) -> CreateComponentFunctionResult { return context.createComponent<ScriptTargetComponent>(e, node); };
	result["Network"] = [] (const EntityFactoryContext& context, EntityRef& e, const ConfigNode& node) -> CreateComponentFunctionResult { return context.createComponent<NetworkComponent>(e, node); };
	return result;
}


using ComponentReflectorList = std::vector<std::unique_ptr<ComponentReflector>>;

static ComponentReflectorList makeComponentReflectors() {
	ComponentReflectorList result;
	result.reserve(11);
	result.push_back(std::make_unique<ComponentReflectorImpl<Transform2DComponent>>());
	result.push_back(std::make_unique<ComponentReflectorImpl<SpriteComponent>>());
	result.push_back(std::make_unique<ComponentReflectorImpl<TextLabelComponent>>());
	result.push_back(std::make_unique<ComponentReflectorImpl<SpriteAnimationComponent>>());
	result.push_back(std::make_unique<ComponentReflectorImpl<CameraComponent>>());
	result.push_back(std::make_unique<ComponentReflectorImpl<ParticlesComponent>>());
	result.push_back(std::make_unique<ComponentReflectorImpl<AudioListenerComponent>>());
	result.push_back(std::make_unique<ComponentReflectorImpl<AudioSourceComponent>>());
	result.push_back(std::make_unique<ComponentReflectorImpl<ScriptComponent>>());
	result.push_back(std::make_unique<ComponentReflectorImpl<ScriptTargetComponent>>());
	result.push_back(std::make_unique<ComponentReflectorImpl<NetworkComponent>>());
	return result;
}

namespace Halley {
	std::unique_ptr<System> createSystem(String name) {
		static SystemFactoryMap factories = makeSystemFactories();
		auto result = factories.find(name);
		if (result == factories.end()) {
			throw Exception("System not found: " + name, HalleyExceptions::Entity);
		}
		return std::unique_ptr<System>(result->second());
	}

	CreateComponentFunctionResult createComponent(const EntityFactoryContext& context, const String& name, EntityRef& entity, const ConfigNode& componentData) {
		static ComponentFactoryMap factories = makeComponentFactories();
		auto result = factories.find(name);
		if (result == factories.end()) {
			throw Exception("Component not found: " + name, HalleyExceptions::Entity);
		}
		return result->second(context, entity, componentData);
	}

	ComponentReflector& getComponentReflector(int componentId) {
		static ComponentReflectorList reflectors = makeComponentReflectors();
		return *reflectors.at(componentId);
	}
}
