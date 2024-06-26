#pragma once
#include "halley/data_structures/vector.h"
#include "halley/api/audio_api.h"

namespace Halley
{
	struct AudioBuffer
	{
		Vector<AudioSample> samples;

		AudioBuffer(size_t size);
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
		AudioSamples getSpan() const;

	private:
		AudioBuffer* buffer;
		AudioBufferPool* pool;

		void clear();
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

		gsl::span<AudioBuffer*> getBuffers();
		AudioMultiChannelSamples getSpans() const;
		AudioMultiChannelSamples getSampleSpans() const;

		AudioBuffer& operator[](size_t n) const;

		bool matches(size_t n, size_t len) const;
		void clear();

	private:
		std::array<AudioBuffer*, AudioConfig::maxChannels> buffers;
		AudioMultiChannelSamples spans;
		AudioMultiChannelSamples sampleSpans;
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
		struct Table {
			Vector<std::unique_ptr<AudioBuffer>> entries;
			Vector<AudioBuffer*> available;
		};

		std::array<Table, 16> buffersTable;

		AudioBuffer& allocBuffer(size_t numSamples);
	};
}
