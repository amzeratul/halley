#include "audio_buffer.h"

using namespace Halley;

AudioBufferRef::AudioBufferRef(AudioBuffer& buffer, AudioBufferPool& pool)
	: buffer(&buffer)
	, pool(pool)
{
}

AudioBufferRef::AudioBufferRef(AudioBufferRef&& other) noexcept
	: buffer(other.buffer)
	, pool(other.pool)
{
	other.buffer = nullptr;
}

AudioBufferRef::~AudioBufferRef()
{
	if (buffer) {
		pool.returnBuffer(*buffer);
		buffer = nullptr;
	}
}

AudioBuffer& AudioBufferRef::get() const
{
	return *buffer;
}

AudioBufferRef AudioBufferPool::getBuffer(size_t numSamples)
{
	for (auto& b: buffers) {
		if (b.available && b.buffer->packs.size() * AudioSamplePack::NumSamples >= numSamples) {
			b.available = false;
			return AudioBufferRef(*b.buffer, *this);
		}
	}

	// Couldn't find a free one, create new
	size_t allocSize = nextPowerOf2((numSamples + AudioSamplePack::NumSamples - 1) / AudioSamplePack::NumSamples);
	buffers.push_back(Entry(std::make_unique<AudioBuffer>()));
	buffers.back().buffer->packs.resize(allocSize);
	return AudioBufferRef(*buffers.back().buffer, *this);
}

void AudioBufferPool::returnBuffer(AudioBuffer& buffer)
{
	for (auto& b: buffers) {
		if (b.buffer.get() == &buffer) {
			Expects(!b.available);
			b.available = true;
		}
	}
}

AudioBufferPool::Entry::Entry(std::unique_ptr<AudioBuffer>&& buffer)
	: available(false)
	, buffer(std::move(buffer))
{
}
