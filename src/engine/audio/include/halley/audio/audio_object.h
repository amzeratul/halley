#pragma once

#include "halley/resources/resource.h"
#include "halley/resources/resource_data.h"
#include "halley/core/api/audio_api.h"
#include "halley/maths/range.h"

namespace Halley {
	class Random;

	class AudioObject final : public Resource {
    public:
    	AudioObject();
		explicit AudioObject(const ConfigNode& config);

        void loadLegacyEvent(const ConfigNode& node);

        uint32_t getAudioObjectId() const;

		const String& getGroup() const;
        std::shared_ptr<const AudioClip> getRandomClip(Random& rng) const;

		Range<float> getPitch() const;
		Range<float> getVolume() const;
		float getDelay() const;
		bool getLoop() const;
    	
    	void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		void reload(Resource&& resource) override;
        static std::shared_ptr<AudioObject> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::AudioObject; }
    	void loadDependencies(Resources& resources);

    private:
		uint32_t audioObjectId;
		
		Vector<String> clips;
		Vector<std::shared_ptr<const AudioClip>> clipData;
		String group;
		Range<float> pitch;
		Range<float> volume;
		float delay = 0.0f;
		bool loop = false;

		void generateId();
    };
}
