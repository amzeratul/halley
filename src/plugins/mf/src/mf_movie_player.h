#pragma once
#include "halley/core/api/movie_api.h"
#include <memory>
#include "halley/resources/resource_data.h"
#include "halley/time/halleytime.h"
#include "halley/core/graphics/texture.h"
#include "halley/core/graphics/sprite/sprite.h"
#include <halley/audio/audio_clip.h>
#include "halley/core/graphics/movie/movie_player.h"

#include <Mfapi.h>
#include <Mfidl.h>
#include <Mfreadwrite.h>
#include <Mferror.h>
#undef min
#undef max

namespace Halley
{
	class VideoAPI;

	class MFMoviePlayer : public MoviePlayer
	{
	public:
		MFMoviePlayer(VideoAPI& video, AudioAPI& audio, std::shared_ptr<ResourceDataStream> data);
		~MFMoviePlayer() noexcept;

	protected:
		void requestVideoFrame() override;
		void requestAudioFrame() override;
		void onReset() override;

	private:
		std::shared_ptr<ResourceDataStream> data;

		IMFByteStream* inputByteStream = nullptr;
		IMFSourceReader *reader = nullptr;
		IMFSourceReaderCallback* sampleReceiver = nullptr;
		int minStride;

		void init();
		void deInit();

		HRESULT onReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample* pSample);

		void readVideoSample(Time time, const gsl::byte* data, int stride);
		void readAudioSample(Time time, gsl::span<const gsl::byte> data);
	};
}
