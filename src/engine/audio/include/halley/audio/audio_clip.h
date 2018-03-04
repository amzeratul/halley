#pragma once
#include "halley/resources/resource.h"
#include "halley/resources/resource_data.h"
#include "halley/core/api/audio_api.h"

namespace Halley
{
	class ResourceLoader;
	class VorbisData;

	class IAudioClip
	{
	public:
		virtual ~IAudioClip() = default;

		virtual size_t copyChannelData(size_t channelN, size_t pos, size_t len, gsl::span<AudioConfig::SampleFormat> dst) const = 0;
		virtual size_t getNumberOfChannels() const = 0;
		virtual size_t getLength() const = 0; // in samples
		virtual size_t getLoopPoint() const { return 0; } // in samples
		virtual bool isLoaded() const { return true; }
	};

	class AudioClip : public AsyncResource, public IAudioClip
	{
	public:
		AudioClip(size_t numChannels);
		~AudioClip();

		void loadFromStatic(std::shared_ptr<ResourceDataStatic> data, Metadata meta);
		void loadFromStream(std::shared_ptr<ResourceDataStream> data, Metadata meta);

		size_t copyChannelData(size_t channelN, size_t pos, size_t len, gsl::span<AudioConfig::SampleFormat> dst) const override;
		size_t getNumberOfChannels() const override;
		size_t getLength() const override; // in samples
		size_t getLoopPoint() const override; // in samples
		bool isLoaded() const override;

		static std::shared_ptr<AudioClip> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::AudioClip; }

	private:
		size_t sampleLength = 0;
		size_t numChannels = 0;
		size_t loopPoint = 0;
		mutable size_t streamPos = 0;
		bool streaming = false;

		// TODO: sort this mess?
		mutable std::vector<std::vector<AudioConfig::SampleFormat>> temp0;
		mutable std::vector<std::vector<AudioConfig::SampleFormat>> temp1;
		mutable std::vector<std::vector<AudioConfig::SampleFormat>> samples;
		mutable std::unique_ptr<VorbisData> vorbisData;
	};
}
