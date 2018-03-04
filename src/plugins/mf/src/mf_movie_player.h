#pragma once
#include "halley/core/api/movie_api.h"
#include <memory>
#include "halley/resources/resource_data.h"

#include <Mfapi.h>
#include <Mfidl.h>
#include <Mfreadwrite.h>
#include "halley/time/halleytime.h"

namespace Halley
{
	class MFMoviePlayer : public MoviePlayer
	{
	public:
		MFMoviePlayer(std::shared_ptr<ResourceDataStream> data);
		~MFMoviePlayer() noexcept;

		void play() override;
		void pause() override;
		void reset();

		void update(Time t) override;

		MoviePlayerState getState() const override;
		Vector2f getSize() const override;

		HRESULT onReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample* pSample);

	private:
		std::shared_ptr<ResourceDataStream> data;
		IMFByteStream* inputByteStream = nullptr;
		IMFSourceReader *reader = nullptr;
		IMFSourceReaderCallback* sampleReceiver = nullptr;
		MoviePlayerState state = MoviePlayerState::Uninitialised;

		std::vector<int> videoStreams;
		std::vector<int> audioStreams;

		Time time = 0;
		Time nextTime = 0;

		void init();
		void deInit();
	};

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
}
