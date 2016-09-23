#pragma once
#include "halley/resources/resource.h"
#include "halley/resources/resource_data.h"
#include "halley/core/api/audio_api.h"

namespace Halley
{
	class ResourceLoader;

	class AudioClip : public AsyncResource
	{
	public:
		AudioClip();

		void loadFromData(std::shared_ptr<ResourceDataStatic> data);

		void getChannelData(size_t channelN, size_t pos, gsl::span<AudioConfig::SampleFormat> dst) const;
		size_t getLength() const; // in samples
		size_t getNumberOfChannels() const;

		static std::shared_ptr<AudioClip> loadResource(ResourceLoader& loader);

	private:
		size_t sampleLength;
		std::vector<std::vector<AudioConfig::SampleFormat>> samples;
	};
}
