#include "mf_movie_player.h"
#include "halley/resources/resource_data.h"
#include "halley/core/api/audio_api.h"
#include "halley/core/api/video_api.h"
#include "resource_data_byte_stream.h"
#include "halley/core/graphics/texture_descriptor.h"
#include "halley/concurrency/concurrent.h"
#include "halley/core/resources/resources.h"
#include "halley/support/logger.h"
#include "halley/core/game/game_platform.h"
#include "halley/maths/random.h"

using namespace Halley;

/*
#include <D3D9.h>
#pragma comment(lib, "Mfuuid.lib")
#pragma comment(lib, "Mf.lib")
#pragma comment(lib, "strmiids.lib")
*/

MFMoviePlayer::MFMoviePlayer(VideoAPI& video, AudioAPI& audio, std::shared_ptr<ResourceDataStream> data)
	: MoviePlayer(video, audio)
	, data(std::move(data))
{
	init();
}

MFMoviePlayer::~MFMoviePlayer() noexcept
{
	reset();
	deInit();
}

static String guidToString(GUID guid)
{
	OLECHAR* guidString;
	StringFromCLSID(guid, &guidString);
	auto result = String(guidString);
	::CoTaskMemFree(guidString);
	return result;
}

void MFMoviePlayer::init()
{
#ifdef WINDOWS_STORE
	inputByteStream = new ResourceDataByteStream(data);
#else
	std::array<wchar_t, 256> tmpPath;
	GetTempPathW(DWORD(tmpPath.size()), tmpPath.data());
	tempFileName = String(tmpPath.data()) + "\\hlyvid" + toString(int(Random::getGlobal().getRawInt())) + ".mp4";
	auto tmpFile = CreateFileW(tempFileName.getUTF16().c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
	auto fullData = data->getReader()->readAll();
	DWORD remaining = DWORD(fullData.size());
	DWORD pos = 0;
	while (remaining > 0) {
		DWORD written;
		if (!WriteFile(tmpFile, fullData.data() + pos, remaining, &written, nullptr)) {
			throw Exception("Unable to initialise video player", HalleyExceptions::MoviePlugin);
		}
		pos += written;
		remaining -= written;
	}
	FlushFileBuffers(tmpFile);
	CloseHandle(tmpFile);

	{
		auto hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_NONE, tempFileName.getUTF16().c_str(), &inputByteStream);
		if (!SUCCEEDED(hr)) {
			throw Exception("Unable to open media file", HalleyExceptions::MoviePlugin);
		}
	}
#endif
	inputByteStream->AddRef();

	IMFAttributes* attributes = nullptr;
	HRESULT hr = MFCreateAttributes(&attributes, 1);
	if (!SUCCEEDED(hr)) {
		throw Exception("Unable to create attributes", HalleyExceptions::MoviePlugin);
	}

	/*
	constexpr bool useAsync = false;
	if (useAsync) {
		sampleReceiver = new MoviePlayerSampleReceiver(*this);
		sampleReceiver->AddRef();
		attributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, sampleReceiver);
	}
	*/

	// DX11 acceleration
	IMFDXGIDeviceManager* deviceManager = nullptr;
	/*
	auto dx11Device = static_cast<IUnknown*>(getVideoAPI().getImplementationPointer("ID3D11Device"));
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
	*/

	hr = MFCreateSourceReaderFromByteStream(inputByteStream, attributes, &reader);
	if (!SUCCEEDED(hr)) {
		throw Exception("Unable to create source reader", HalleyExceptions::MoviePlugin);
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
		streams.emplace_back();
		auto& curStream = streams.back();

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
						throw Exception("Unable to read major type", HalleyExceptions::MoviePlugin);
					}

					MFCreateMediaType(&targetType);
					hr = targetType->SetGUID(MF_MT_MAJOR_TYPE, majorType);
					if (!SUCCEEDED(hr)) {
						throw Exception("Unable to write major type", HalleyExceptions::MoviePlugin);
					}

					if (majorType == MFMediaType_Video) {
						UINT64 frameSize;
						nativeType->GetUINT64(MF_MT_FRAME_SIZE, &frameSize);
						auto videoSize = Vector2i(int(frameSize >> 32), int(frameSize & 0xFFFFFFFFull));
						UINT64 aspectRatioRaw;
						nativeType->GetUINT64(MF_MT_PIXEL_ASPECT_RATIO, &aspectRatioRaw);
						float par = float(aspectRatioRaw >> 32) / float(aspectRatioRaw & 0xFFFFFFFFull);

						Logger::logInfo("Video stream found with majorType " + guidToString(majorType) + ", frameSize " + toString(videoSize) );

						uint32_t stride;
						hr = nativeType->GetUINT32(MF_MT_DEFAULT_STRIDE, &stride);

						setVideoSize(videoSize);
						curStream.type = MoviePlayerStreamType::Video;
						subType = MFVideoFormat_NV12; // NV12 is the only format supported by DX accelerated decoding

						if (SUCCEEDED(hr)) {
							minStride = int(stride);
						} else {
							minStride = -1;
							auto origSubType = GUID_NULL;
							hr = nativeType->GetGUID(MF_MT_SUBTYPE, &origSubType);
							Logger::logInfo("Original subType: " + guidToString(origSubType));

							if (SUCCEEDED(hr)) {
#ifndef WINDOWS_STORE
								LONG tmp;
								hr = MFGetStrideForBitmapInfoHeader(subType.Data1, videoSize.x, &tmp);
								if (SUCCEEDED(hr)) {
									minStride = int(tmp);
								}
#endif
							}
							if (minStride == -1) {
								if (subType == MFVideoFormat_NV12) {
									Logger::logInfo("Setting stride based on NV12 video format");
									minStride = videoSize.x;
								} else if (subType == MFVideoFormat_RGB32) {
									Logger::logInfo("Setting stride based on RGB32 video format");
									minStride = videoSize.x * 4;
								} else {
									Logger::logInfo("Unknown type: " + guidToString(subType));
									minStride = videoSize.x;
								}
							}
						}
					} else if (majorType == MFMediaType_Audio) {
						UINT32 sampleRate;
						UINT32 numChannels;
						nativeType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &sampleRate);
						nativeType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &numChannels);

						curStream.type = MoviePlayerStreamType::Audio;
						subType = MFAudioFormat_PCM;
					}

					hr = targetType->SetGUID(MF_MT_SUBTYPE, subType);
					if (!SUCCEEDED(hr)) {
						throw Exception("Unable to write subtype", HalleyExceptions::MoviePlugin);
					}

					hr = reader->SetCurrentMediaType(streamIndex, nullptr, targetType);
					if (!SUCCEEDED(hr)) {
						throw Exception("Unable to set current media type", HalleyExceptions::MoviePlugin);
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
				throw Exception("Error reading stream info", HalleyExceptions::MoviePlugin);
			}
		}
	}

	reset();
}

void MFMoviePlayer::deInit()
{
	if (sampleReceiver) {
		sampleReceiver->Release();
		sampleReceiver = nullptr;
	}
	if (reader) {
		reader->Release();
		reader = nullptr;
	}
	if (inputByteStream) {
		inputByteStream->Release();
		inputByteStream = nullptr;
	}
#ifndef WINDOWS_STORE
	if (!tempFileName.isEmpty()) {
		DeleteFileW(tempFileName.getUTF16().c_str());
	}
#endif
}

void MFMoviePlayer::requestVideoFrame()
{
	const DWORD controlFlags = 0;
	DWORD streamIndex;
	DWORD streamFlags;
	LONGLONG timestamp;
	IMFSample* sample;
	auto hr = reader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, controlFlags, &streamIndex, &streamFlags, &timestamp, &sample);
	onReadSample(hr, streamIndex, streamFlags, timestamp, sample);
}

void MFMoviePlayer::requestAudioFrame()
{
	const DWORD controlFlags = 0;
	DWORD streamIndex;
	DWORD streamFlags;
	LONGLONG timestamp;
	IMFSample* sample;
	auto hr = reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, controlFlags, &streamIndex, &streamFlags, &timestamp, &sample);
	onReadSample(hr, streamIndex, streamFlags, timestamp, sample);
}

void MFMoviePlayer::onReset()
{
	PROPVARIANT pos;
	pos.vt = VT_I8;
	pos.hVal.QuadPart = 0;
	reader->SetCurrentPosition(GUID_NULL, pos);
}

HRESULT MFMoviePlayer::onReadSample(HRESULT hr, DWORD streamIndex, DWORD streamFlags, LONGLONG timestamp, IMFSample* sample)
{
	auto& curStream = streams.at(streamIndex);
	if (streamFlags & MF_SOURCE_READERF_ENDOFSTREAM) {
		curStream.eof = true;
	}

	if (sample) {
		Time sampleTime = Time(timestamp) / 10000000.0;

		DWORD bufferCount;
		sample->GetBufferCount(&bufferCount);
		for (int i = 0; i < int(bufferCount); ++i) {
			IMFMediaBuffer* buffer;
			sample->GetBufferByIndex(i, &buffer);
			DWORD length;
			buffer->GetCurrentLength(&length);

			if (curStream.type == MoviePlayerStreamType::Video) {
				/*
				IDirect3DSurface9 *surface = nullptr;
				auto hr = MFGetService(buffer, MR_BUFFER_SERVICE, __uuidof(IDirect3DSurface9), reinterpret_cast<void**>(&surface));
				if (SUCCEEDED(hr)) {
					std::cout << Release();
				}"Got DX9 surface!\n";
				surface->Release();
				*/
				
				IMF2DBuffer* buffer2d = nullptr;
				hr = buffer->QueryInterface(__uuidof(IMF2DBuffer), reinterpret_cast<void**>(&buffer2d));
				if (SUCCEEDED(hr)) {
					BYTE* src;
					LONG pitch;
					buffer2d->Lock2D(&src, &pitch);
					readVideoSample(sampleTime, reinterpret_cast<gsl::byte*>(src), pitch);
					buffer2d->Unlock2D();
					buffer2d->Release();
				} else if (hr == E_NOINTERFACE) {
					BYTE* src;
					DWORD maxLen;
					DWORD curLen;
					buffer->Lock(&src, &maxLen, &curLen);
					readVideoSample(sampleTime, reinterpret_cast<gsl::byte*>(src), minStride);
					buffer->Unlock();
				} else {
					throw Exception("Error while querying for 2D buffer: " + toString(hr), HalleyExceptions::MoviePlugin);
				}
			}

			if (curStream.type == MoviePlayerStreamType::Audio) {
				BYTE* data;
				DWORD maxLen;
				DWORD curLen;
				buffer->Lock(&data, &maxLen, &curLen);
				readAudioSample(sampleTime, gsl::as_bytes(gsl::span<const BYTE>(data, curLen)));
				buffer->Unlock();
			}

			buffer->Release();
		}

		sample->Release();
	}

	return S_OK;
}

void MFMoviePlayer::readVideoSample(Time time, const gsl::byte* data, int stride)
{
	if (!data) {
		throw Exception("Null pointer provided to MFMoviePlayer::readVideoSample.", HalleyExceptions::MoviePlugin);
	}
	if (stride <= 0) {
		throw Exception("MFMoviePlayer::readVideoSample, Stride is not positive: " + toString(stride), HalleyExceptions::MoviePlugin);
	}

	const auto videoSize = getSize();
	const int yPlaneHeight = alignUp(videoSize.y, 16);
	const int uvPlaneHeight = alignUp(videoSize.y / 2, 16);
	const int width = alignUp(videoSize.x, 16);
	const int height = yPlaneHeight + uvPlaneHeight;

	auto srcData = gsl::span<const gsl::byte>(data, stride * height);
	Bytes myData(srcData.size_bytes());
	memcpy(myData.data(), data, myData.size());

	TextureDescriptor descriptor;
	descriptor.format = TextureFormat::Indexed;
	descriptor.pixelFormat = PixelDataFormat::Image;
	descriptor.size = Vector2i(width, height);
	descriptor.pixelData = TextureDescriptorImageData(std::move(myData), stride);

	onVideoFrameAvailable(time, std::move(descriptor));
}

void MFMoviePlayer::readAudioSample(Time time, gsl::span<const gsl::byte> data)
{
	auto src = gsl::span<const short>(reinterpret_cast<const short*>(data.data()), data.size() / sizeof(short));

	std::vector<AudioConfig::SampleFormat> samples(src.size());
	for (int i = 0; i < src.size(); ++i) {
		samples[i] = src[i] / 32768.0f;
	}

	onAudioFrameAvailable(time, samples);
}
