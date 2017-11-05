#pragma once
#include <vector>
#include "halley/core/api/audio_api.h"

namespace Halley
{
	struct AudioBuffer
	{
		std::vector<AudioSamplePack> packs;
	};

	class AudioBufferPool;

	class AudioBufferRef
	{
	public:
		AudioBufferRef();
		AudioBufferRef(AudioBuffer& buffer, AudioBufferPool& pool);
		AudioBufferRef(const AudioBufferRef& other) = delete;
		AudioBufferRef(AudioBufferRef&& other) noexcept;

		AudioBufferRef& operator=(const AudioBufferRef& other) = delete;
		AudioBufferRef& operator=(AudioBufferRef&& other) noexcept;

		~AudioBufferRef();

		AudioBuffer& getBuffer() const;
		gsl::span<AudioSamplePack> getSpan() const;
		gsl::span<AudioConfig::SampleFormat> getSampleSpan() const;

	private:
		AudioBuffer* buffer;
		AudioBufferPool* pool;
	};

	class AudioBuffersRef
	{
	public:
		AudioBuffersRef();
		AudioBuffersRef(size_t n, std::array<AudioBuffer*, AudioConfig::maxChannels> buffers, AudioBufferPool& pool);
		AudioBuffersRef(const AudioBuffersRef& other) = delete;
		AudioBuffersRef(AudioBuffersRef&& other) noexcept;

		AudioBuffersRef& operator=(const AudioBuffersRef& other) = delete;
		AudioBuffersRef& operator=(AudioBuffersRef&& other) noexcept;

		~AudioBuffersRef();

		std::array<gsl::span<AudioSamplePack>, AudioConfig::maxChannels> getSpans() const;
		std::array<gsl::span<AudioConfig::SampleFormat>, AudioConfig::maxChannels> getSampleSpans() const;

	private:
		std::array<AudioBuffer*, AudioConfig::maxChannels> buffers;
		std::array<gsl::span<AudioSamplePack>, AudioConfig::maxChannels> spans;
		std::array<gsl::span<AudioConfig::SampleFormat>, AudioConfig::maxChannels> sampleSpans;
		size_t nBuffers;
		AudioBufferPool* pool;
	};

	class AudioBufferPool
	{
	public:
		AudioBufferRef getBuffer(size_t numSamples);
		AudioBuffersRef getBuffers(size_t n, size_t numSamples);
		void returnBuffer(AudioBuffer& buffer);

	private:
		struct Entry
		{
			size_t size = 0;
			bool available = false;
			std::unique_ptr<AudioBuffer> buffer;

			explicit Entry(std::unique_ptr<AudioBuffer>&& buffer);
		};

		std::array<std::vector<Entry>, 16> buffersTable;

		AudioBuffer& allocBuffer(size_t numSamples);
	};
}
