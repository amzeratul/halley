#pragma once
#include "halley/core/api/movie_api.h"
#include <memory>
#include "halley/resources/resource_data.h"
#include "halley/time/halleytime.h"
#include "halley/core/graphics/texture.h"
#include "halley/core/graphics/sprite/sprite.h"

#include <Mfapi.h>
#include <Mfidl.h>
#include <Mfreadwrite.h>
#include <Mferror.h>
#undef min
#undef max

namespace Halley
{
	class VideoAPI;

	enum class MoviePlayerStreamType
	{
		Unknown,
		Video,
		Audio,
		Other
	};

	class MoviePlayerStream
	{
	public:
		MoviePlayerStreamType type = MoviePlayerStreamType::Unknown;
		bool playing = true;
		bool eof = false;
	};

	struct PendingFrame
	{
		std::shared_ptr<Texture> texture;
		Time time;
	};
	
	class MFMoviePlayer : public MoviePlayer
	{
	public:
		MFMoviePlayer(VideoAPI& video, AudioAPI& audio, std::shared_ptr<ResourceDataStream> data);
		~MFMoviePlayer() noexcept;

		void play() override;
		void pause() override;
		void reset();

		void update(Time t) override;
		Sprite getSprite(Resources& resources) override;

		MoviePlayerState getState() const override;
		Vector2i getSize() const override;

	private:
		VideoAPI& video;
		AudioAPI& audio;
		std::shared_ptr<ResourceDataStream> data;

		IMFByteStream* inputByteStream = nullptr;
		IMFSourceReader *reader = nullptr;
		IMFSourceReaderCallback* sampleReceiver = nullptr;
		MoviePlayerState state = MoviePlayerState::Uninitialised;

		std::vector<MoviePlayerStream> streams;

		Vector2i videoSize;
		std::shared_ptr<Texture> currentTexture;
		std::list<PendingFrame> pendingFrames;

		Time time = 0;

		void init();
		void deInit();

		HRESULT onReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample* pSample);

		void readVideoSample(Time time, gsl::span<const gsl::byte> data, int stride);
		void readAudioSample(Time time, gsl::span<const gsl::byte> data);
	};

	/*
	class MoviePlayerSampleReceiver final : public IMFSourceReaderCallback2
	{
	public:
		explicit MoviePlayerSampleReceiver(MFMoviePlayer& player);

		HRESULT QueryInterface(const IID& riid, void** ppvObject) override;
		ULONG AddRef() override;
		ULONG Release() override;
		HRESULT OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample* pSample) override;
		HRESULT OnFlush(DWORD dwStreamIndex) override;
		HRESULT OnEvent(DWORD dwStreamIndex, IMFMediaEvent* pEvent) override;
		HRESULT OnTransformChange() override;
		HRESULT OnStreamError(DWORD dwStreamIndex, HRESULT hrStatus) override;

	private:
		MFMoviePlayer& player;
		long refCount = 0;
	};
	*/
}
