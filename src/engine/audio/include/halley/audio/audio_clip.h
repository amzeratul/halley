#pragma once
#include "halley/resources/resource.h"
#include "halley/resources/resource_data.h"
#include "halley/core/api/audio_api.h"

namespace Halley
{
	class ResourceLoader;
	class VorbisData;

	class AudioClip : public AsyncResource
	{
	public:
		AudioClip(size_t numChannels);
		~AudioClip();

		void loadFromStatic(std::shared_ptr<ResourceDataStatic> data, Metadata meta);
		void loadFromStream(std::shared_ptr<ResourceDataStream> data, Metadata meta);

		gsl::span<const AudioConfig::SampleFormat> getChannelData(size_t channelN, size_t pos, size_t len) const;

		size_t getLength() const; // in samples
		size_t getNumberOfChannels() const;
		size_t getLoopPoint() const; // in samples
		static std::shared_ptr<AudioClip> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::AudioClip; }

	private:
		size_t sampleLength;
		size_t numChannels;
		size_t loopPoint = 0;
		mutable size_t streamPos = 0;
		bool streaming;

		// TODO: sort this mess?
		mutable std::vector<std::vector<AudioConfig::SampleFormat>> temp0;
		mutable std::vector<std::vector<AudioConfig::SampleFormat>> temp1;
		mutable std::vector<std::vector<AudioConfig::SampleFormat>> samples;
		mutable std::unique_ptr<VorbisData> vorbisData;
	};
}
