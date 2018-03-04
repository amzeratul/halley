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

	private:
		std::shared_ptr<ResourceDataStream> data;
		IMFByteStream* inputByteStream = nullptr;
		IMFSourceReader *reader = nullptr;
		MoviePlayerState state = MoviePlayerState::Uninitialised;

		std::vector<int> videoStreams;
		std::vector<int> audioStreams;

		Time time = 0;

		void init();
		void deInit();
	};
}
