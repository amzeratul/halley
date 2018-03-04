#include "mf_movie_player.h"
#include "halley/resources/resource_data.h"
#include "halley/core/api/audio_api.h"
#include "halley/core/api/video_api.h"
#include <Mferror.h>
#include "resource_data_byte_stream.h"
using namespace Halley;

MFMoviePlayer::MFMoviePlayer(VideoAPI& video, AudioAPI& audio, std::shared_ptr<ResourceDataStream> data)
	: video(video)
	, audio(audio)
	, data(std::move(data))
{
	init();
}

MFMoviePlayer::~MFMoviePlayer() noexcept
{
	deInit();
}

void MFMoviePlayer::play()
{
	if (state == MoviePlayerState::Paused) {
		state = MoviePlayerState::Playing;
	}
}

void MFMoviePlayer::pause()
{
	if (state == MoviePlayerState::Playing) {
		state = MoviePlayerState::Paused;
	}
}

void MFMoviePlayer::reset()
{
	state = MoviePlayerState::Paused;
	time = 0;
}

void MFMoviePlayer::update(Time t)
{
	if (state == MoviePlayerState::Playing) {
		time += t;

		if (true || time >= nextTime) {
			const DWORD controlFlags = 0;

			DWORD streamIndex;
			DWORD streamFlags;
			LONGLONG timestamp;
			IMFSample* sample;

			for (int i = 0; i < 10; ++i) {
				auto hr = reader->ReadSample(MF_SOURCE_READER_ANY_STREAM, controlFlags, &streamIndex, &streamFlags, &timestamp, &sample);
				if (sample) {
					std::cout << "[t = " << time << "] ";
				}
				onReadSample(hr, streamIndex, streamFlags, timestamp, sample);
			}
		}
	}
}

MoviePlayerState MFMoviePlayer::getState() const
{
	return state;
}

Vector2f MFMoviePlayer::getSize() const
{
	return {};
}

void MFMoviePlayer::init()
{
	/*
	// HACK
	HRESULT hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_NOBUFFERING, L"c:\\users\\amz\\desktop\\test.mp4", &inputByteStream);
	if (!SUCCEEDED(hr)) {
		throw Exception("Unable to load media file");
	}
	*/
	inputByteStream = new ResourceDataByteStream(data);
	inputByteStream->AddRef();

	IMFAttributes* attributes = nullptr;
	HRESULT hr = MFCreateAttributes(&attributes, 1);
	if (!SUCCEEDED(hr)) {
		throw Exception("Unable to create attributes");
	}

	constexpr bool useAsync = false;
	if (useAsync) {
		//sampleReceiver = new MoviePlayerSampleReceiver(*this);
		//sampleReceiver->AddRef();
		//attributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, sampleReceiver);
	}

	// DX11 acceleration
	auto dx11Device = nullptr;//static_cast<IUnknown*>(video.getImplementationPointer("ID3D11Device"));
	IMFDXGIDeviceManager* deviceManager = nullptr;
	if (dx11Device) {
		UINT resetToken;
		hr = MFCreateDXGIDeviceManager(&resetToken, &deviceManager);
		if (!SUCCEEDED(hr)) {
			throw Exception("Unable to create DXGI Device Manager");
		}
		hr = deviceManager->ResetDevice(dx11Device, resetToken);
		if (!SUCCEEDED(hr)) {
			throw Exception("Unable to reset DXGI Device Manager with device");
		}
		attributes->SetUnknown(MF_SOURCE_READER_D3D_MANAGER, deviceManager);
	}

	hr = MFCreateSourceReaderFromByteStream(inputByteStream, attributes, &reader);
	if (!SUCCEEDED(hr)) {
		throw Exception("Unable to create source reader");
	}

	// Release these temporaries
	if (attributes) {
		attributes->Release();
	}
	if (deviceManager) {
		deviceManager->Release();
	}

	reader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS, false);
	reader->SetStreamSelection(MF_SOURCE_READER_FIRST_VIDEO_STREAM, true);
	reader->SetStreamSelection(MF_SOURCE_READER_FIRST_AUDIO_STREAM, true);

	// Setup the right decoding formats
	bool hasMoreStreams = true;
	for (int streamIndex = 0; hasMoreStreams; ++streamIndex) {
		for (int mediaTypeIndex = 0; ; ++mediaTypeIndex) {
			IMFMediaType *nativeType = nullptr;
			hr = reader->GetNativeMediaType(streamIndex, mediaTypeIndex, &nativeType);

			if (hr == MF_E_INVALIDSTREAMNUMBER) {
				hasMoreStreams = false;
				break;
			} else if (hr == MF_E_NO_MORE_TYPES) {
				break;
			} else if (SUCCEEDED(hr)) {
				GUID majorType;
				GUID subType;
				IMFMediaType *targetType = nullptr;

				try {
					hr = nativeType->GetGUID(MF_MT_MAJOR_TYPE, &majorType);
					if (!SUCCEEDED(hr)) {
						throw Exception("Unable to read major type");
					}

					MFCreateMediaType(&targetType);
					hr = targetType->SetGUID(MF_MT_MAJOR_TYPE, majorType);
					if (!SUCCEEDED(hr)) {
						throw Exception("Unable to write major type");
					}

					if (majorType == MFMediaType_Video) {
						UINT64 frameSize;
						nativeType->GetUINT64(MF_MT_FRAME_SIZE, &frameSize);
						auto size = Vector2i(int(frameSize >> 32), int(frameSize & 0xFFFFFFFFull));
						std::cout << "Video is " << size.x << " x " << size.y << std::endl;

						videoStreams.push_back(streamIndex);
						subType = MFVideoFormat_NV12; // NV12 is the only format supported by DX accelerated decoding
					} else if (majorType == MFMediaType_Audio) {
						UINT32 sampleRate;
						UINT32 numChannels;
						nativeType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &sampleRate);
						nativeType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &numChannels);
						std::cout << "Audio has " << numChannels << " channels at " << sampleRate << " Hz." << std::endl;

						audioStreams.push_back(streamIndex);
						subType = MFAudioFormat_PCM;
					}

					hr = targetType->SetGUID(MF_MT_SUBTYPE, subType);
					if (!SUCCEEDED(hr)) {
						throw Exception("Unable to write subtype");
					}

					hr = reader->SetCurrentMediaType(streamIndex, nullptr, targetType);
					if (!SUCCEEDED(hr)) {
						throw Exception("Unable to set current media type");
					}
				} catch (...) {
					if (targetType) {
						targetType->Release();
					}
					nativeType->Release();
					throw;
				}

				targetType->Release();
				nativeType->Release();
			} else {
				throw Exception("Error reading stream info");
			}
		}
	}

	reset();
}

void MFMoviePlayer::deInit()
{
	/*
	if (sampleReceiver) {
		sampleReceiver->Release();
		sampleReceiver = nullptr;
	}
	*/
	if (reader) {
		reader->Release();
		reader = nullptr;
	}
	if (inputByteStream) {
		inputByteStream->Release();
		inputByteStream = nullptr;
	}
}

HRESULT MFMoviePlayer::onReadSample(HRESULT hr, DWORD streamIndex, DWORD streamFlags, LONGLONG timestamp, IMFSample* sample)
{
	if (sample) {
		std::cout << "Read sample on " << streamIndex << ", flags = " << streamFlags << ", timestamp = " << (Time(timestamp) / 10000000.0) << std::endl;
		if (streamIndex == 0) {
			nextTime = Time(timestamp) / 10000000.0;
		}
		sample->Release();
	}

	return S_OK;
}

/*
MoviePlayerSampleReceiver::MoviePlayerSampleReceiver(MFMoviePlayer& player)
	: player(player)
{
}


HRESULT MoviePlayerSampleReceiver::OnReadSample(HRESULT hr, DWORD streamIndex, DWORD streamFlags, LONGLONG timestamp, IMFSample* sample)
{
	return player.onReadSample(hr, streamIndex, streamFlags, timestamp, sample);
}

HRESULT MoviePlayerSampleReceiver::QueryInterface(const IID& riid, void** ppvObject)
{
	if (!ppvObject) {
		return E_INVALIDARG;
	}
	*ppvObject = nullptr;

	if (riid == __uuidof(IMFSourceReaderCallback)) {
		AddRef();
		*ppvObject = static_cast<IMFSourceReaderCallback*>(this);
		return NOERROR;
	} else if (riid == __uuidof(IMFSourceReaderCallback2)) {
		AddRef();
		*ppvObject = static_cast<IMFSourceReaderCallback2*>(this);
		return NOERROR;
	} else {
		return E_NOINTERFACE;
	}
}

ULONG MoviePlayerSampleReceiver::AddRef()
{
	return InterlockedIncrement(&refCount);
}

ULONG MoviePlayerSampleReceiver::Release()
{
	ULONG uCount = InterlockedDecrement(&refCount);
    if (uCount == 0) {
		delete this;
    }
    return uCount;
}

HRESULT MoviePlayerSampleReceiver::OnFlush(DWORD dwStreamIndex)
{
	return S_OK;
}

HRESULT MoviePlayerSampleReceiver::OnEvent(DWORD dwStreamIndex, IMFMediaEvent* pEvent)
{
	return S_OK;
}

HRESULT MoviePlayerSampleReceiver::OnTransformChange()
{
	return S_OK;
}

HRESULT MoviePlayerSampleReceiver::OnStreamError(DWORD dwStreamIndex, HRESULT hrStatus)
{
	return S_OK;
}
*/
