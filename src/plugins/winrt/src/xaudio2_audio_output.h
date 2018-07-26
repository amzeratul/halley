#pragma once
#include "halley/core/api/halley_api_internal.h"
#include "xaudio2.h"

namespace Halley
{
	class XAudio2AudioOutput;

	class XAudio2MasteringVoice
	{
	public:
		XAudio2MasteringVoice(XAudio2AudioOutput& audio, const AudioSpec& spec, String deviceId);
		~XAudio2MasteringVoice();

	private:
		IXAudio2MasteringVoice* masteringVoice = nullptr;
	};

	class XAudio2SourceVoice : public IXAudio2VoiceCallback
	{
	public:
		XAudio2SourceVoice(XAudio2AudioOutput& audio, const AudioSpec& spec, AudioCallback callback);
		~XAudio2SourceVoice();

		void queueAudio(gsl::span<const float> samples);
		void play();

		void __stdcall OnVoiceProcessingPassStart(UINT32 BytesRequired) override;
		void __stdcall OnVoiceProcessingPassEnd() override;

		void __stdcall OnStreamEnd() override;

		void __stdcall OnBufferStart(void* pBufferContext) override;
		void __stdcall OnBufferEnd(void* pBufferContext) override;

		void __stdcall OnLoopEnd(void* pBufferContext) override;

		void __stdcall OnVoiceError(void* pBufferContext, HRESULT Error) override;

	private:
		bool running;
		XAudio2AudioOutput& audio;
		AudioCallback callback;

		IXAudio2SourceVoice* voice = nullptr;

		std::condition_variable condition;
		std::mutex mutex;
	};

	class XAudio2AudioDevice : public AudioDevice
	{
	public:
		XAudio2AudioDevice(const String& name);

		String getName() const override;

	private:
		String name;
	};

	class XAudio2AudioOutput : public AudioOutputAPIInternal
	{
	public:
		void init() override;
		void deInit() override;

		Vector<std::unique_ptr<const AudioDevice>> getAudioDevices() override;
		AudioSpec openAudioDevice(const AudioSpec& requestedFormat, const AudioDevice* device, AudioCallback prepareAudioCallback) override;
		void closeAudioDevice() override;

		void startPlayback() override;
		void stopPlayback() override;

		void queueAudio(gsl::span<const float> data) override;
		bool needsMoreAudio() override;

		bool needsAudioThread() const override;

		IXAudio2& getXAudio2();

	private:
		IXAudio2* xAudio2 = nullptr;
		std::unique_ptr<XAudio2MasteringVoice> masterVoice;
		std::unique_ptr<XAudio2SourceVoice> voice;
		AudioCallback callback;
		AudioSpec format;
	};
}
