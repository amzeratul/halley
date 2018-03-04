#include "mf_movie_player.h"
#include "halley/resources/resource_data.h"
#include <Mfapi.h>
#include <Mfidl.h>
#include <Mfreadwrite.h>
#include <Mferror.h>
using namespace Halley;

MFMoviePlayer::MFMoviePlayer(std::shared_ptr<ResourceDataStream> data)
	: data(std::move(data))
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

		const DWORD controlFlags = 0;
		DWORD streamFlags;
		DWORD streamIndex;
		LONGLONG timestamp;
		IMFSample* sample;
		reader->ReadSample(MF_SOURCE_READER_ANY_STREAM, controlFlags, &streamIndex, &streamFlags, &timestamp, &sample);

		if (sample) {
			std::cout << "Read sample on " << streamIndex << std::endl;
			sample->Release();
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
	// HACK
	HRESULT hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_NOBUFFERING, L"c:\\users\\amz\\desktop\\test.mp4", &inputByteStream);
	if (!SUCCEEDED(hr)) {
		throw Exception("Unable to load media file");
	}

	IMFAttributes* attributes = nullptr;

	// DX11 acceleration
	IMFDXGIDeviceManager* deviceManager = nullptr;
	UINT resetToken;
	hr = MFCreateDXGIDeviceManager(&resetToken, &deviceManager);
	if (!SUCCEEDED(hr)) {
		throw Exception("Unable to create DXGI Device Manager");
	}
	MFCreateAttributes(&attributes, 1);
	attributes->SetUnknown(MF_SOURCE_READER_D3D_MANAGER, deviceManager);

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
						videoStreams.push_back(streamIndex);
						subType = MFVideoFormat_NV12; // NV12 is the only format supported by DX accelerated decoding
					} else if (majorType == MFMediaType_Audio) {
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
	if (reader) {
		reader->Release();
		reader = nullptr;
	}
	if (inputByteStream) {
		inputByteStream->Release();
		inputByteStream = nullptr;
	}
}
