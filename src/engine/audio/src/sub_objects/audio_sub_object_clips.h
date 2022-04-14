#pragma once
#include "audio_sub_object.h"

namespace Halley {
	class AudioSubObjectClips final : public IAudioSubObject {
	public:
		void load(const ConfigNode& node) override;

		AudioSubObjectType getType() override { return AudioSubObjectType::Clips; }

		std::unique_ptr<AudioSource> makeSource(AudioEngine& engine, AudioEmitter& emitter) const override;
		void loadDependencies(Resources& resources) override;

		void serialize(Serializer& s) const override;
		void deserialize(Deserializer& s) override;

	private:
		Vector<String> clips;
		Vector<std::shared_ptr<const AudioClip>> clipData;
		float delay = 0.0f;
		bool loop = false;
		int loopStart = 0;
		int loopEnd = 0;
	};
}
