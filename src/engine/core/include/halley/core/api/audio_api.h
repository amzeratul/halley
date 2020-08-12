#pragma once

#include <functional>
#include <gsl/gsl>
#include <memory>
#include <vector>
#include "halley/text/halleystring.h"
#include "halley/maths/vector2.h"
#include "halley/maths/vector3.h"

#include "halley/data_structures/ring_buffer.h"

namespace Halley
{
	class AudioPosition;
	class AudioClip;
	class AudioEvent;
	class IAudioClip;
	class AudioVoiceBehaviour;
	class AudioEngine;

    namespace AudioConfig {
        constexpr int sampleRate = 48000;
		constexpr int maxChannels = 8;
    	constexpr int maxVoices = 128;
        using SampleFormat = float;
    }

	class AudioDevice
	{
	public:
		virtual ~AudioDevice() {}
		virtual String getName() const = 0;
	};

	struct alignas(64) AudioSamplePack
	{
		constexpr static int NumSamples = 16;
		std::array<AudioConfig::SampleFormat, NumSamples> samples; // AVX-512 friendly
	};

	enum class AudioSampleFormat
	{
		Undefined,
		Int16,
		Int32,
		Float
	};

	template <>
	struct EnumNames<AudioSampleFormat> {
		constexpr std::array<const char*, 4> operator()() const {
			return{{
				"undefined",
				"int16",
				"int32",
				"float"
			}};
		}
	};

	class AudioSpec
	{
	public:
		int sampleRate = 0;
		int numChannels = 0;
		int bufferSize = 0;
		AudioSampleFormat format = AudioSampleFormat::Undefined;

		AudioSpec() {}
		AudioSpec(int sampleRate, int numChannels, int bufferSize, AudioSampleFormat format)
			: sampleRate(sampleRate)
			, numChannels(numChannels)
			, bufferSize(bufferSize)
			, format(format)
		{}
	};

	class AudioListenerData
	{
	public:
		Vector3f position;
		float referenceDistance = 100.0f;

		AudioListenerData() {}
		AudioListenerData(Vector3f position, float referenceDistance = 100.0f)
			: position(position)
			, referenceDistance(referenceDistance)
		{}
	};

	class AudioChannelData
	{
	public:
		float pan;
		float gain = 1.0f;
	};

	using AudioCallback = std::function<void()>;

	class IAudioOutput
	{
	public:
		virtual ~IAudioOutput() = default;
		
		virtual size_t getAvailable() = 0;
		virtual size_t output(gsl::span<gsl::byte> dst, bool padWithZeroes) = 0;
	};

	class AudioOutputAPI
	{
	public:		
		virtual ~AudioOutputAPI() = default;

		virtual Vector<std::unique_ptr<const AudioDevice>> getAudioDevices() = 0;
		virtual AudioSpec openAudioDevice(const AudioSpec& requestedFormat, const AudioDevice* device = nullptr, AudioCallback prepareAudioCallback = AudioCallback()) = 0;
		virtual void closeAudioDevice() = 0;

		virtual void startPlayback() = 0;
		virtual void stopPlayback() = 0;

		virtual void onAudioAvailable() = 0;

		virtual bool needsMoreAudio() = 0;
		virtual bool needsAudioThread() const = 0;
		virtual bool needsInterleavedSamples() const { return true; }

	protected:
		static size_t getAudioBytesNeeded(const AudioSpec& outputSpec, size_t nBuffers)
		{
			const size_t sizePerSample = outputSpec.format == AudioSampleFormat::Int16 ? 2 : 4;
			return nBuffers * outputSpec.bufferSize * outputSpec.numChannels * sizePerSample;
		}
		
		IAudioOutput& getAudioOutputInterface() const { return *audioOutputInterface; }
		
	private:
		friend class AudioEngine;
		
		IAudioOutput* audioOutputInterface = nullptr;
		
		void setAudioOutputInterface(IAudioOutput& aoInterface) { audioOutputInterface = &aoInterface; }
	};

	class IAudioHandle
	{
	public:
		virtual ~IAudioHandle() {}

		virtual void setGain(float gain) = 0;
		virtual void setVolume(float volume) = 0;
		virtual void setPosition(Vector2f pos) = 0;
		virtual void setPan(float pan) = 0;

		virtual void stop(float fadeTime = 0.0f) = 0;
		virtual bool isPlaying() const = 0;
		virtual void addBehaviour(std::unique_ptr<AudioVoiceBehaviour> behaviour) = 0;
	};
	using AudioHandle = std::shared_ptr<IAudioHandle>;

	class AudioAPI
	{
	public:
		virtual ~AudioAPI() {}

		virtual Vector<std::unique_ptr<const AudioDevice>> getAudioDevices() = 0;
		virtual void startPlayback(int deviceNumber = 0) = 0;
		virtual void stopPlayback() = 0;
		virtual void pausePlayback() = 0;
		virtual void resumePlayback() = 0;

		virtual AudioHandle postEvent(const String& name, AudioPosition position) = 0;

		virtual AudioHandle play(std::shared_ptr<const IAudioClip> clip, AudioPosition position, float volume = 1.0f, bool loop = false) = 0;
		virtual AudioHandle playMusic(const String& eventName, int track = 0, float fadeInTime = 0.0f) = 0;
		virtual AudioHandle getMusic(int track = 0) = 0;
		virtual void stopMusic(int track = 0, float fadeOutTime = 0.0f) = 0;
		virtual void stopAllMusic(float fadeOutTime = 0.0f) = 0;

		virtual void setMasterVolume(float gain = 1.0f) = 0;
		virtual void setGroupVolume(const String& groupName, float gain = 1.0f) = 0;
		virtual void setOutputChannels(std::vector<AudioChannelData> audioChannelData) = 0;

		virtual void setListener(AudioListenerData listener) = 0;
	};
}
