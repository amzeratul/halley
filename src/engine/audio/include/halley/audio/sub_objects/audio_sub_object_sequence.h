#pragma once
#include "../audio_fade.h"
#include "../audio_sub_object.h"

namespace Halley {
    class AudioSubObjectSequence final : public IAudioSubObject {
    public:
   	    void load(const ConfigNode& node) override;
    	ConfigNode toConfigNode() const override;

        AudioSubObjectType getType() const override { return AudioSubObjectType::Sequence; }
	    std::unique_ptr<AudioSource> makeSource(AudioEngine& engine, AudioEmitter& emitter) const override;

        String getName() const override;
		size_t getNumSubObjects() const override;
		AudioSubObjectHandle& getSubObject(size_t n) override;
		bool canAddObject(AudioSubObjectType type, const std::optional<String>& caseName) const override;
		void addObject(AudioSubObjectHandle handle, const std::optional<String>& caseName, size_t idx) override;
		AudioSubObjectHandle removeObject(const IAudioObject* object) override;

        void loadDependencies(Resources& resources) override;
	    void serialize(Serializer& s) const override;
	    void deserialize(Deserializer& s) override;

        AudioFade& getCrossFade();

    private:
        Vector<AudioSubObjectHandle> segments;

        AudioFade crossFade;
    };
}
