// Halley codegen version 123
#pragma once

#include <halley.hpp>


#include "components/audio_listener_component.h"
#include "halley/entity/components/transform_2d_component.h"
#include "components/audio_source_component.h"
#include "components/velocity_component.h"
#include "system_messages/play_network_sound_system_message.h"

// Generated file; do not modify.
template <typename T>
class AudioSystemBase : private Halley::System {
public:
	class ListenerFamily : public Halley::FamilyBaseOf<ListenerFamily> {
	public:
		AudioListenerComponent& audioListener;
		const Transform2DComponent& transform2D;
	
		using Type = Halley::FamilyType<AudioListenerComponent, Transform2DComponent>;
	
	protected:
		ListenerFamily(AudioListenerComponent& audioListener, const Transform2DComponent& transform2D)
			: audioListener(audioListener)
			, transform2D(transform2D)
		{
		}
	};

	class SourceFamily : public Halley::FamilyBaseOf<SourceFamily> {
	public:
		AudioSourceComponent& audioSource;
		const Transform2DComponent& transform2D;
		const Halley::MaybeRef<VelocityComponent> velocity{};
	
		using Type = Halley::FamilyType<AudioSourceComponent, Transform2DComponent, Halley::MaybeRef<VelocityComponent>>;
	
	protected:
		SourceFamily(AudioSourceComponent& audioSource, const Transform2DComponent& transform2D, const Halley::MaybeRef<VelocityComponent> velocity)
			: audioSource(audioSource)
			, transform2D(transform2D)
			, velocity(velocity)
		{
		}
	};

	virtual void onMessageReceived(const PlayNetworkSoundSystemMessage& msg) = 0;

	AudioSystemBase()
		: System({&listenerFamily, &sourceFamily}, {})
	{
		static_assert(std::is_final_v<T>, "System must be final.");
	}
protected:
	const Halley::HalleyAPI& getAPI() const {
		return doGetAPI();
	}
	Halley::FamilyBinding<ListenerFamily> listenerFamily{};
	Halley::FamilyBinding<SourceFamily> sourceFamily{};

private:
	friend Halley::System* halleyCreateAudioSystem();

	void initBase() override final {
		invokeInit<T>(static_cast<T*>(this));
		initialiseFamilyBinding<T, ListenerFamily>(listenerFamily, static_cast<T*>(this));
		initialiseFamilyBinding<T, SourceFamily>(sourceFamily, static_cast<T*>(this));
	}

	void updateBase(Halley::Time time) override final {
		static_cast<T*>(this)->update(time);
	}

	void onSystemMessageReceived(const Halley::SystemMessageContext& context) override final {
		switch (context.msgId) {
		case PlayNetworkSoundSystemMessage::messageIndex: {
		    auto& realMsg = reinterpret_cast<PlayNetworkSoundSystemMessage&>(*context.msg);
		    static_cast<T*>(this)->onMessageReceived(realMsg);
		    if (context.callback) {
		        context.callback(nullptr, {});
		    }
		    break;
		}
		}
	}
	bool canHandleSystemMessage(int msgIndex, const Halley::String& targetSystem) const override final {
		if (!targetSystem.isEmpty() && targetSystem != getName()) return false;
		switch (msgIndex) {
		case PlayNetworkSoundSystemMessage::messageIndex: return true;
		}
		return false;
	}
};

