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

gsl::span<AudioSamplePack> AudioBufferRef::getSpan() const
{
	return gsl::span<AudioSamplePack>(buffer->packs);
}

gsl::span<AudioConfig::SampleFormat> AudioBufferRef::getSampleSpan() const
{
	return gsl::span<AudioConfig::SampleFormat>(buffer->packs.data()->samples.data(), buffer->packs.size() * AudioSamplePack::NumSamples);
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
		spans[i] = buffers[i]->packs;
		sampleSpans[i] = gsl::span<AudioConfig::SampleFormat>(spans[i].data()->samples.data(), spans[i].size() * AudioSamplePack::NumSamples);
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

std::array<gsl::span<AudioSamplePack>, AudioConfig::maxChannels> AudioBuffersRef::getSpans() const
{
	return spans;
}

std::array<gsl::span<AudioConfig::SampleFormat>, AudioConfig::maxChannels> AudioBuffersRef::getSampleSpans() const
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
	Expects(AudioSamplePack::NumSamples == 16);
	Expects(numSamples >= AudioSamplePack::NumSamples);
	Expects(numSamples < 65536);

	constexpr size_t log2NumSamples = 4;
	size_t idx = fastLog2(numSamples) - log2NumSamples;
	auto& buffers = buffersTable[idx];

	for (auto& b: buffers) {
		if (b.available) {
			b.available = false;
			return *b.buffer;
		}
	}

	// Couldn't find a free one, create new
	buffers.push_back(Entry(std::make_unique<AudioBuffer>()));
	buffers.back().buffer->packs.resize(1LL << idx);
	return *buffers.back().buffer;
}

void AudioBufferPool::returnBuffer(AudioBuffer& buffer)
{
	size_t idx = fastLog2(buffer.packs.size());
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
