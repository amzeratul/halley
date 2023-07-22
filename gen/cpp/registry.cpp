// Halley codegen version 123
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
#include "components/sprite_animation_replicator_component.h"
#include "messages/start_script_message.h"
#include "messages/terminate_script_message.h"
#include "messages/terminate_scripts_with_tag_message.h"
#include "messages/send_script_msg_message.h"
#include "messages/play_animation_message.h"
#include "messages/play_animation_once_message.h"
#include "messages/stop_particles_message.h"
#include "system_messages/terminate_scripts_with_tag_system_message.h"
#include "system_messages/network_entity_lock_system_message.h"

// System factory functions
System* halleyCreateNetworkLockSystem();
System* halleyCreateNetworkReceiveSystem();
System* halleyCreateNetworkSendSystem();
System* halleyCreateParticleSystem();
System* halleyCreateScriptSystem();
System* halleyCreateSpriteAnimationSystem();


class GameCodegenFunctions : public CodegenFunctions {
public:
	Vector<SystemReflector> makeSystemReflectors() override {
		Vector<SystemReflector> result;
		result.reserve(6);
		result.push_back(SystemReflector("NetworkLockSystem", &halleyCreateNetworkLockSystem));
		result.push_back(SystemReflector("NetworkReceiveSystem", &halleyCreateNetworkReceiveSystem));
		result.push_back(SystemReflector("NetworkSendSystem", &halleyCreateNetworkSendSystem));
		result.push_back(SystemReflector("ParticleSystem", &halleyCreateParticleSystem));
		result.push_back(SystemReflector("ScriptSystem", &halleyCreateScriptSystem));
		result.push_back(SystemReflector("SpriteAnimationSystem", &halleyCreateSpriteAnimationSystem));
		return result;
	}
	Vector<std::unique_ptr<ComponentReflector>> makeComponentReflectors() override {
		Vector<std::unique_ptr<ComponentReflector>> result;
		result.reserve(14);
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
		result.push_back(std::make_unique<ComponentReflectorImpl<SpriteAnimationReplicatorComponent>>());
		return result;
	}
	Vector<std::unique_ptr<MessageReflector>> makeMessageReflectors() override {
		Vector<std::unique_ptr<MessageReflector>> result;
		result.reserve(7);
		result.push_back(std::make_unique<MessageReflectorImpl<StartScriptMessage>>());
		result.push_back(std::make_unique<MessageReflectorImpl<TerminateScriptMessage>>());
		result.push_back(std::make_unique<MessageReflectorImpl<TerminateScriptsWithTagMessage>>());
		result.push_back(std::make_unique<MessageReflectorImpl<SendScriptMsgMessage>>());
		result.push_back(std::make_unique<MessageReflectorImpl<PlayAnimationMessage>>());
		result.push_back(std::make_unique<MessageReflectorImpl<PlayAnimationOnceMessage>>());
		result.push_back(std::make_unique<MessageReflectorImpl<StopParticlesMessage>>());
		return result;
	}
	Vector<std::unique_ptr<SystemMessageReflector>> makeSystemMessageReflectors() override {
		Vector<std::unique_ptr<SystemMessageReflector>> result;
		result.reserve(2);
		result.push_back(std::make_unique<SystemMessageReflectorImpl<TerminateScriptsWithTagSystemMessage>>());
		result.push_back(std::make_unique<SystemMessageReflectorImpl<NetworkEntityLockSystemMessage>>());
		return result;
	}
};

namespace Halley {
	std::unique_ptr<CodegenFunctions> createCodegenFunctions() {
		return std::make_unique<GameCodegenFunctions>();
	}
}
