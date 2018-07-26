#ifdef WINDOWS_STORE

#include "xaudio2_audio_output.h"
#include "Xaudio2.h"
using namespace Halley;

XAudio2MasteringVoice::XAudio2MasteringVoice(XAudio2AudioOutput& audio, const AudioSpec& spec, String deviceId)
{
	auto result = audio.getXAudio2().CreateMasteringVoice(&masteringVoice, spec.numChannels, spec.sampleRate, 0, deviceId.isEmpty() ? nullptr : deviceId.getUTF16().c_str());
	if (result != S_OK) {
		throw Exception("Unable to create mastering voice");
	}	
}

XAudio2MasteringVoice::~XAudio2MasteringVoice()
{
	if (masteringVoice) {
		masteringVoice->DestroyVoice();
		masteringVoice = nullptr;
	}
}

XAudio2SourceVoice::XAudio2SourceVoice(XAudio2AudioOutput& audio, const AudioSpec& spec, AudioCallback callback)
	: running(false)
	, audio(audio)
	, callback(callback)
{
	WAVEFORMATEX format;
	ZeroMemory(&format, sizeof(format));

	format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
	format.nChannels = spec.numChannels;
	format.wBitsPerSample = 32;
	format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
	format.nSamplesPerSec = spec.sampleRate;
	format.nAvgBytesPerSec = format.nChannels * format.nSamplesPerSec * 4;
	format.cbSize = 0;

	auto result = audio.getXAudio2().CreateSourceVoice(&voice, &format, 0, 1.0, this);
	if (result != S_OK) {
		throw Exception("Unable to create source voice");
	}
}

XAudio2SourceVoice::~XAudio2SourceVoice()
{
	running = false;
	voice->Stop();
	voice->FlushSourceBuffers();

	if (false) {
		std::unique_lock<std::mutex> lock(mutex);
		condition.wait(lock);
	}

	voice->DestroyVoice();
}

void XAudio2SourceVoice::queueAudio(gsl::span<const float> samples)
{
	XAUDIO2_BUFFER buffer;
	ZeroMemory(&buffer, sizeof(buffer));

	float* data = new float[samples.size()];
	memcpy_s(data, samples.size_bytes(), samples.data(), samples.size_bytes());

	buffer.Flags = 0;
	buffer.AudioBytes = samples.size_bytes();
	buffer.pAudioData = reinterpret_cast<const BYTE*>(data);
	buffer.PlayBegin = 0;
	buffer.PlayLength = 0;
	buffer.LoopBegin = 0;
	buffer.LoopLength = 0;
	buffer.LoopCount = 0;
	buffer.pContext = data;

	voice->SubmitSourceBuffer(&buffer, nullptr);
}

void XAudio2SourceVoice::play()
{
	running = true;
	auto result = voice->Start();
	if (result != S_OK) {
		throw Exception("Unable to start voice playback");
	}
	callback();
}

void XAudio2SourceVoice::OnVoiceProcessingPassStart(UINT32 BytesRequired) {}
void XAudio2SourceVoice::OnVoiceProcessingPassEnd() {}
void XAudio2SourceVoice::OnStreamEnd()
{
	if (!running) {
		condition.notify_one();
	}
}

void XAudio2SourceVoice::OnBufferStart(void* pBufferContext)
{
	if (running) {
		callback();
	}
}

void XAudio2SourceVoice::OnBufferEnd(void* pBufferContext)
{
	delete[] reinterpret_cast<float*>(pBufferContext);
}

void XAudio2SourceVoice::OnLoopEnd(void* pBufferContext) {}
void XAudio2SourceVoice::OnVoiceError(void* pBufferContext, HRESULT Error) {}

XAudio2AudioDevice::XAudio2AudioDevice(const String& name)
	: name(name)
{
}

String XAudio2AudioDevice::getName() const
{
	return name;
}

void XAudio2AudioOutput::init()
{
	HRESULT result = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	if (result != S_OK) {
		throw Exception("Unable to initialise XAudio2");
	}
}

void XAudio2AudioOutput::deInit()
{
	if (xAudio2) {
		xAudio2->Release();
		xAudio2 = nullptr;
	}
}

Vector<std::unique_ptr<const AudioDevice>> XAudio2AudioOutput::getAudioDevices()
{
	Vector<std::unique_ptr<const AudioDevice>> results;
	results.emplace_back(std::make_unique<XAudio2AudioDevice>(""));
	return results;
}

AudioSpec XAudio2AudioOutput::openAudioDevice(const AudioSpec& requestedFormat, const AudioDevice* device, AudioCallback prepareAudioCallback)
{
	masterVoice = std::make_unique<XAudio2MasteringVoice>(*this, requestedFormat, device->getName());
	callback = prepareAudioCallback;
	format = requestedFormat;

	return requestedFormat;
}

void XAudio2AudioOutput::closeAudioDevice()
{
	callback = {};
	masterVoice.reset();
}

void XAudio2AudioOutput::startPlayback()
{
	voice = std::make_unique<XAudio2SourceVoice>(*this, format, callback);
	voice->play();
}

void XAudio2AudioOutput::stopPlayback()
{
	voice.reset();
}

void XAudio2AudioOutput::queueAudio(gsl::span<const float> data)
{
	if (voice) {
		voice->queueAudio(data);
	}
}

bool XAudio2AudioOutput::needsMoreAudio()
{
	return false;
}

bool XAudio2AudioOutput::needsAudioThread() const
{
	return false;
}

IXAudio2& XAudio2AudioOutput::getXAudio2()
{
	return *xAudio2;
}

#endif