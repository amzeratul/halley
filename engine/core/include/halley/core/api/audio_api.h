#pragma once

#include <functional>
#include <gsl/gsl>
#include <memory>
#include <vector>

namespace Halley
{
	class AudioDevice
	{
	public:
		virtual ~AudioDevice() {}
		virtual String getName() const = 0;
	};

	using AudioCallback = std::function<void(gsl::span<gsl::byte> span)>;

	enum class AudioSampleFormat
	{
		Undefined,
		Int16,
		Int32,
		Float
	};

	class AudioSpec
	{
	public:
		int sampleRate;
		int numChannels;
		int bufferSize;
		AudioSampleFormat format;

		AudioSpec() {}
		AudioSpec(int sampleRate, int numChannels, int bufferSize, AudioSampleFormat format)
			: sampleRate(sampleRate)
			, numChannels(numChannels)
			, bufferSize(bufferSize)
			, format(format)
		{}
	};

	class AudioOutputAPI
	{
	public:
		virtual ~AudioOutputAPI() {}

		virtual Vector<std::unique_ptr<const AudioDevice>> getAudioDevices() = 0;
		virtual AudioSpec openAudioDevice(const AudioSpec& requestedFormat, AudioCallback callback, const AudioDevice* device = nullptr) = 0;
		virtual void closeAudioDevice() = 0;

		virtual void startPlayback() = 0;
		virtual void stopPlayback() = 0;
	};
}
