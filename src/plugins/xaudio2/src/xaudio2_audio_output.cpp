#include "xaudio2_audio_output.h"
#include "Xaudio2.h"
using namespace Halley;

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

XAudio2MasteringVoice::XAudio2MasteringVoice(XAudio2AudioOutput& audio, const AudioSpec& spec, String deviceId)
{
	auto result = audio.getXAudio2().CreateMasteringVoice(&masteringVoice, spec.numChannels, spec.sampleRate, 0, deviceId.isEmpty() ? nullptr : deviceId.getUTF16().c_str());
	if (result != S_OK) {
		throw Exception("Unable to create mastering voice", HalleyExceptions::AudioOutPlugin);
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

	buffers.resize(4);

	auto result = audio.getXAudio2().CreateSourceVoice(&voice, &format, 0, 1.0, this);
	if (result != S_OK) {
		throw Exception("Unable to create source voice", HalleyExceptions::AudioOutPlugin);
	}
}

XAudio2SourceVoice::~XAudio2SourceVoice()
{
	running = false;
	voice->Stop();
	voice->FlushSourceBuffers();
	voice->DestroyVoice();
}

void XAudio2SourceVoice::queueAudio(gsl::span<const float> samples)
{
	XAUDIO2_BUFFER buffer;
	ZeroMemory(&buffer, sizeof(buffer));

	auto* data = getBuffer(samples.size());
	memcpy_s(data->buffer.data(), samples.size_bytes(), samples.data(), samples.size_bytes());

	buffer.Flags = 0;
	buffer.AudioBytes = static_cast<UINT32>(samples.size_bytes());
	buffer.pAudioData = reinterpret_cast<const BYTE*>(data->buffer.data());
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
		throw Exception("Unable to start voice playback", HalleyExceptions::AudioOutPlugin);
	}
	callback();
}

uint64_t XAudio2SourceVoice::getSamplesPlayed() const
{
	XAUDIO2_VOICE_STATE state;
	voice->GetState(&state);
	return state.SamplesPlayed;
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
	returnBuffer(static_cast<Buffer*>(pBufferContext));
}

void XAudio2SourceVoice::OnLoopEnd(void* pBufferContext) {}
void XAudio2SourceVoice::OnVoiceError(void* pBufferContext, HRESULT Error) {}

XAudio2SourceVoice::Buffer* XAudio2SourceVoice::getBuffer(size_t size)
{
	for (auto& b: buffers) {
		if (!b.busy) {
			b.busy = true;
			b.buffer.resize(size);
			return &b;
		}
	}
	return nullptr;
}

void XAudio2SourceVoice::returnBuffer(Buffer* buffer)
{
	buffer->busy = false;
}

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
		throw Exception("Unable to initialise XAudio2", HalleyExceptions::AudioOutPlugin);
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
	voice = std::make_unique<XAudio2SourceVoice>(*this, format, [this] ()
	{
		consumeAudio();
	});
	voice->play();
}

void XAudio2AudioOutput::stopPlayback()
{
	voice.reset();
}

void XAudio2AudioOutput::onAudioAvailable()
{
}

bool XAudio2AudioOutput::needsMoreAudio()
{
	return getAudioOutputInterface().getAvailable() < getAudioBytesNeeded(format, 2);
}

bool XAudio2AudioOutput::needsAudioThread() const
{
	return true;
}

uint64_t XAudio2AudioOutput::getSamplesPlayed() const
{
	//return voice->getSamplesPlayed();

	const auto now = std::chrono::high_resolution_clock::now();
	uint64_t pos;
	uint64_t duration;
	{
		std::unique_lock<std::mutex> lock(playbackMutex);
		pos = playbackPos;
		duration = std::chrono::duration(now - lastSubmissionTime).count();
	}
	return pos + static_cast<uint64_t>(duration / 1'000'000'000.0 * format.sampleRate + 0.5);
}

uint64_t XAudio2AudioOutput::getSamplesSubmitted() const
{
	return samplesSubmitted;
}

uint64_t XAudio2AudioOutput::getSamplesLeft() const
{
	const auto now = std::chrono::high_resolution_clock::now();
	int64_t pos;
	int64_t duration;
	int64_t submitted;
	{
		std::unique_lock<std::mutex> lock(playbackMutex);
		pos = static_cast<int64_t>(playbackPos);
		duration = std::chrono::duration(now - lastSubmissionTime).count();
		submitted = static_cast<int64_t>(samplesSubmitted);
	}
	const auto samplesPlayed = pos + static_cast<int64_t>(duration / 1'000'000'000.0 * format.sampleRate + 0.5);
	return std::max(0ll, submitted - samplesPlayed);
}

IXAudio2& XAudio2AudioOutput::getXAudio2()
{
	return *xAudio2;
}

void XAudio2AudioOutput::consumeAudio()
{
	if (!voice) {
		return;
	}

	auto sendBuffer = [&]()
	{
		const auto prevSubmitted = samplesSubmitted.load();
		samplesSubmitted = prevSubmitted + buffer.size() / format.numChannels;
		voice->queueAudio(buffer.span());
		const auto samplesPlayed = voice->getSamplesPlayed();

		std::unique_lock<std::mutex> lock(playbackMutex);
		playbackPos = samplesPlayed;
		lastSubmissionTime = std::chrono::high_resolution_clock::now();
	};

	const size_t availableBytes = getAudioOutputInterface().getAvailable();
	if (availableBytes > 0) {
		buffer.resize(std::min(static_cast<size_t>(format.bufferSize * format.numChannels), availableBytes / sizeof(float)));
		getAudioOutputInterface().output(buffer.byte_span(), true);
		sendBuffer();
	} else {
		// Insert silence
		buffer.resize(128, 0.0f);
		sendBuffer();
	}
}
