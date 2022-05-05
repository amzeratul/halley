#pragma once
#include "../audio_sub_object.h"

namespace Halley {
	class AudioSubObjectClips final : public IAudioSubObject {
	public:
		AudioSubObjectClips() = default;
		explicit AudioSubObjectClips(std::shared_ptr<const AudioClip> clip);

		void load(const ConfigNode& node) override;
		ConfigNode toConfigNode() const override;
		void toLegacyConfigNode(ConfigNode& dst) const;

		AudioSubObjectType getType() const override { return AudioSubObjectType::Clips; }

		String getName() const override;
		gsl::span<const String> getClips() const override;
		bool canCollapseToClip() const override;

		std::unique_ptr<AudioSource> makeSource(AudioEngine& engine, AudioEmitter& emitter) const override;
		void loadDependencies(Resources& resources) override;

		void serialize(Serializer& s) const override;
		void deserialize(Deserializer& s) override;

		bool canAddObject(AudioSubObjectType type, const std::optional<String>& caseName) const override;
		void addObject(AudioSubObjectHandle handle, const std::optional<String>& caseName, size_t idx) override;
		void addClip(std::shared_ptr<const AudioClip> clip, const std::optional<String>& caseName, size_t idx) override;
		void removeClip(const String& clipId) override;
		void swapClips(size_t idxA, size_t idxB) override;

		bool getLoop() const;
		void setLoop(bool loop);
		Range<float>& getGain();
		int getLoopStart() const;
		int getLoopEnd() const;
		void setLoopStart(int samples);
		void setLoopEnd(int samples);

	private:
		Vector<String> clips;
		Vector<std::shared_ptr<const AudioClip>> clipData;
		bool loop = false;
		int loopStart = 0;
		int loopEnd = 0;
		Range<float> gain = Range<float>(1, 1);

		bool depsLoaded = false;
	};
}
