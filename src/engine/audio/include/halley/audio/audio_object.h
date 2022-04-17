#pragma once

#include "audio_sub_object.h"
#include "halley/resources/resource.h"
#include "halley/resources/resource_data.h"
#include "halley/core/api/audio_api.h"
#include "halley/maths/range.h"

namespace Halley {
	class AudioSource;
	class Random;

	class AudioObject final : public Resource {
    public:
    	AudioObject();
		explicit AudioObject(const ConfigNode& config);

        void loadLegacyEvent(const ConfigNode& node);

        AudioObjectId getAudioObjectId() const;
		const String& getGroup() const;
		Range<float> getPitch() const;
		Range<float> getVolume() const;

		std::unique_ptr<AudioSource> makeSource(AudioEngine& engine, AudioEmitter& emitter) const;
    	
    	void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		void reload(Resource&& resource) override;
        static std::shared_ptr<AudioObject> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::AudioObject; }
    	void loadDependencies(Resources& resources);

		void makeDefault();
		ConfigNode toConfigNode() const;
        String toYAML() const;

    private:
		AudioObjectId audioObjectId;
		AudioSubObjectHandle root;
		
		String group;
		Range<float> pitch;
		Range<float> volume;

		void generateId();
    };
}
