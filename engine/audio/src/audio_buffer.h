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
		AudioBufferRef(AudioBuffer& buffer, AudioBufferPool& pool);
		AudioBufferRef(const AudioBufferRef& other) = delete;
		AudioBufferRef(AudioBufferRef&& other) noexcept;

		AudioBufferRef& operator=(const AudioBufferRef& other) = delete;
		AudioBufferRef& operator=(AudioBufferRef&& other) = delete;

		~AudioBufferRef();

		AudioBuffer& getBuffer() const;
		gsl::span<AudioSamplePack> getSpan() const;

	private:
		AudioBuffer* buffer;
		AudioBufferPool& pool;
	};

	class AudioBufferPool
	{
	public:
		AudioBufferRef getBuffer(size_t numSamples);
		void returnBuffer(AudioBuffer& buffer);

	private:
		struct Entry
		{
			bool available = false;
			std::unique_ptr<AudioBuffer> buffer;

			explicit Entry(std::unique_ptr<AudioBuffer>&& buffer);
		};

		std::vector<Entry> buffers;
	};
}
