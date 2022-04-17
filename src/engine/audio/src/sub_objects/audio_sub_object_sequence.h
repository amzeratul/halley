#pragma once
#include "audio_sub_object.h"

namespace Halley {
    class AudioSubObjectSequence final : public IAudioSubObject {
    public:
   	    void load(const ConfigNode& node) override;
    	ConfigNode toConfigNode() const override;

        AudioSubObjectType getType() override { return AudioSubObjectType::Sequence; }
	    std::unique_ptr<AudioSource> makeSource(AudioEngine& engine, AudioEmitter& emitter) const override;

    	void loadDependencies(Resources& resources) override;
	    void serialize(Serializer& s) const override;
	    void deserialize(Deserializer& s) override;
    };
}
