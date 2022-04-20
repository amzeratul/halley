#pragma once
#include "audio_sub_object.h"

namespace Halley {
	class AudioSubObjectClips final : public IAudioSubObject {
	public:
		void load(const ConfigNode& node) override;
		ConfigNode toConfigNode() const override;
		void toLegacyConfigNode(ConfigNode& dst) const;

		AudioSubObjectType getType() override { return AudioSubObjectType::Clips; }

		String getName() const override;
		gsl::span<const String> getClips() const override;
		bool canCollapseToClip() const override;

		std::unique_ptr<AudioSource> makeSource(AudioEngine& engine, AudioEmitter& emitter) const override;
		void loadDependencies(Resources& resources) override;

		void serialize(Serializer& s) const override;
		void deserialize(Deserializer& s) override;

		void addClip(std::shared_ptr<const AudioClip> clip, size_t idx) override;

	private:
		Vector<String> clips;
		Vector<std::shared_ptr<const AudioClip>> clipData;
		bool loop = false;
		int loopStart = 0;
		int loopEnd = 0;
	};
}
