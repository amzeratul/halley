#include "audio_buffer.h"

using namespace Halley;

AudioBufferRef::AudioBufferRef()
	: buffer(nullptr)
	, pool(nullptr)
{	
}

AudioBufferRef::AudioBufferRef(AudioBuffer& buffer, AudioBufferPool& pool)
	: buffer(&buffer)
	, pool(&pool)
{
}

AudioBufferRef::AudioBufferRef(AudioBufferRef&& other) noexcept
	: buffer(other.buffer)
	, pool(other.pool)
{
	other.buffer = nullptr;
	other.pool = nullptr;
}

AudioBufferRef& AudioBufferRef::operator=(AudioBufferRef&& other) noexcept
{
	buffer = other.buffer;
	pool = other.pool;
	other.buffer = nullptr;
	other.pool = nullptr;
	return *this;
}

AudioBufferRef::~AudioBufferRef()
{
	if (buffer && pool) {
		pool->returnBuffer(*buffer);
		buffer = nullptr;
	}
}

AudioBuffer& AudioBufferRef::getBuffer() const
{
	return *buffer;
}

AudioSamples AudioBufferRef::getSpan() const
{
	return AudioSamples(buffer->samples);
}

AudioBuffersRef::AudioBuffersRef()
	: nBuffers(0)
	, pool(nullptr)
{
}

AudioBuffersRef::AudioBuffersRef(size_t n, std::array<AudioBuffer*, AudioConfig::maxChannels> buffers, AudioBufferPool& pool)
	: buffers(buffers)
	, nBuffers(n)
	, pool(&pool)
{
	for (size_t i = 0; i < n; ++i) {
		spans[i] = buffers[i]->samples;
		sampleSpans[i] = AudioSamples(spans[i].data(), spans[i].size());
	}
}

AudioBuffersRef::AudioBuffersRef(AudioBuffersRef&& other) noexcept
{
	buffers = other.buffers;
	nBuffers = other.nBuffers;
	pool = other.pool;

	other.nBuffers = 0;
	other.pool = nullptr;
}

AudioBuffersRef& AudioBuffersRef::operator=(AudioBuffersRef&& other) noexcept
{
	buffers = other.buffers;
	nBuffers = other.nBuffers;
	pool = other.pool;

	other.nBuffers = 0;
	other.pool = nullptr;

	return *this;
}

AudioBuffersRef::~AudioBuffersRef()
{
	if (pool) {
		for (size_t i = 0; i < nBuffers; ++i) {
			pool->returnBuffer(*buffers[i]);
		}
	}
	pool = nullptr;
	nBuffers = 0;
}

gsl::span<AudioBuffer*> AudioBuffersRef::getBuffers()
{
	return gsl::span<AudioBuffer*>(buffers.data(), nBuffers);
}

AudioMultiChannelSamples AudioBuffersRef::getSpans() const
{
	return spans;
}

AudioMultiChannelSamples AudioBuffersRef::getSampleSpans() const
{
	return sampleSpans;
}

AudioBufferRef AudioBufferPool::getBuffer(size_t numSamples)
{
	return AudioBufferRef(allocBuffer(numSamples), *this);
}

AudioBuffersRef AudioBufferPool::getBuffers(size_t n, size_t numSamples)
{
	Expects(n <= AudioConfig::maxChannels);
	std::array<AudioBuffer*, AudioConfig::maxChannels> buffers;
	for (size_t i = 0; i < n; ++i) {
		buffers[i] = &allocBuffer(numSamples);
	}
	return AudioBuffersRef(n, buffers, *this);
}

AudioBuffer& AudioBufferPool::allocBuffer(size_t numSamples)
{
	Expects(numSamples < 65536);

	const size_t idx = fastLog2Ceil(uint32_t(numSamples));
	auto& buffers = buffersTable[idx];

	for (auto& b: buffers) {
		if (b.available) {
			b.available = false;
			return *b.buffer;
		}
	}

	// Couldn't find a free one, create new
	buffers.emplace_back(std::make_unique<AudioBuffer>());
	buffers.back().buffer->samples.resize(1LL << idx);
	return *buffers.back().buffer;
}

void AudioBufferPool::returnBuffer(AudioBuffer& buffer)
{
	const size_t idx = fastLog2Ceil(uint32_t(buffer.samples.size()));
	auto& buffers = buffersTable[idx];

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
