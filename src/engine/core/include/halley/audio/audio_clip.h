#pragma once
#include "halley/resources/resource.h"
#include "halley/resources/resource_data.h"
#include "halley/api/audio_api.h"
#include "audio_buffer.h"

namespace Halley
{
	class AudioBuffersRef;
	class AudioBufferPool;
	class ResourceLoader;
	class VorbisData;
	class AudioClipStreamHandle;

	class IAudioClipStreamHandle {
	public:
		virtual ~IAudioClipStreamHandle() = default;
	};

	class IAudioClip
	{
	public:
		virtual ~IAudioClip() = default;

		virtual bool isLoaded() const { return true; }
		virtual String getName() const { return ""; }

		virtual uint8_t getNumberOfChannels() const = 0;
		virtual size_t getLength() const = 0; // in samples
		virtual size_t getLoopPoint() const { return 0; } // in samples

		virtual bool hasStreamHandles() const { return false; }
		virtual std::unique_ptr<IAudioClipStreamHandle> makeStreamHandle() const { return {}; }

		virtual void prepareChannelData(size_t pos, size_t len, IAudioClipStreamHandle* streamHandle) const = 0;
		virtual size_t copyChannelData(size_t channelN, size_t pos, size_t len, float gain0, float gain1, AudioSamples dst) const = 0;
	};

	class AudioClip final : public AsyncResource, public IAudioClip
	{
		friend class AudioClipStreamHandle;

	public:
		AudioClip(uint8_t numChannels);
		~AudioClip();

		AudioClip& operator=(AudioClip&& other) noexcept;

		void loadFromStatic(std::shared_ptr<ResourceDataStatic> data, Metadata meta);
		void loadFromStream(std::shared_ptr<ResourceDataStream> data, Metadata meta);

		bool isLoaded() const override;
		String getName() const override;

		uint8_t getNumberOfChannels() const override;
		size_t getLength() const override; // in samples
		size_t getLoopPoint() const override; // in samples

		bool hasStreamHandles() const override;
		std::unique_ptr<IAudioClipStreamHandle> makeStreamHandle() const override;

		void prepareChannelData(size_t pos, size_t len, IAudioClipStreamHandle* streamHandle) const override;
		size_t copyChannelData(size_t channelN, size_t pos, size_t len, float gain0, float gain1, AudioSamples dst) const override;

		ResourceMemoryUsage getMemoryUsage() const override;

		static std::shared_ptr<AudioClip> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::AudioClip; }
		void reload(Resource&& resource) override;

	private:
		size_t sampleLength = 0;
		size_t loopPoint = 0;
		uint8_t numChannels = 0;
		bool streaming = false;

		mutable uint8_t readsWithSeek = 0;
		mutable uint8_t readsWithoutSeek = 0;

		std::shared_ptr<ResourceData> streamingResource;

		mutable Vector<Vector<AudioSample>> samples;
		mutable Vector<Vector<AudioSample>> buffer;

		void load(std::shared_ptr<ResourceData> data, Metadata metadata, bool stream);
	};

	class AudioClipStreamHandle : public IAudioClipStreamHandle {
	public:
		AudioClipStreamHandle(const AudioClip& parent);

		void seek(size_t pos);
		void advance(size_t len);
		VorbisData& getVorbisData();

	private:
		std::unique_ptr<VorbisData> vorbisData;
		size_t streamPos = 0;
	};
}
