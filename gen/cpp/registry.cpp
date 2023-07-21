// Halley codegen version 122
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
#include "components/scriptable_component.h"
#include "components/embedded_script_component.h"
#include "components/script_target_component.h"
#include "components/script_tag_target_component.h"
#include "components/network_component.h"

// System factory functions


class GameCodegenFunctions : public CodegenFunctions {
public:
	Vector<SystemReflector> makeSystemReflectors() override {
		Vector<SystemReflector> result;
		result.reserve(0);
		return result;
	}
	Vector<std::unique_ptr<ComponentReflector>> makeComponentReflectors() override {
		Vector<std::unique_ptr<ComponentReflector>> result;
		result.reserve(13);
		result.push_back(std::make_unique<ComponentReflectorImpl<Transform2DComponent>>());
		result.push_back(std::make_unique<ComponentReflectorImpl<SpriteComponent>>());
		result.push_back(std::make_unique<ComponentReflectorImpl<TextLabelComponent>>());
		result.push_back(std::make_unique<ComponentReflectorImpl<SpriteAnimationComponent>>());
		result.push_back(std::make_unique<ComponentReflectorImpl<CameraComponent>>());
		result.push_back(std::make_unique<ComponentReflectorImpl<ParticlesComponent>>());
		result.push_back(std::make_unique<ComponentReflectorImpl<AudioListenerComponent>>());
		result.push_back(std::make_unique<ComponentReflectorImpl<AudioSourceComponent>>());
		result.push_back(std::make_unique<ComponentReflectorImpl<ScriptableComponent>>());
		result.push_back(std::make_unique<ComponentReflectorImpl<EmbeddedScriptComponent>>());
		result.push_back(std::make_unique<ComponentReflectorImpl<ScriptTargetComponent>>());
		result.push_back(std::make_unique<ComponentReflectorImpl<ScriptTagTargetComponent>>());
		result.push_back(std::make_unique<ComponentReflectorImpl<NetworkComponent>>());
		return result;
	}
	Vector<std::unique_ptr<MessageReflector>> makeMessageReflectors() override {
		Vector<std::unique_ptr<MessageReflector>> result;
		result.reserve(0);
		return result;
	}
	Vector<std::unique_ptr<SystemMessageReflector>> makeSystemMessageReflectors() override {
		Vector<std::unique_ptr<SystemMessageReflector>> result;
		result.reserve(0);
		return result;
	}
};

namespace Halley {
	std::unique_ptr<CodegenFunctions> createCodegenFunctions() {
		return std::make_unique<GameCodegenFunctions>();
	}
}
