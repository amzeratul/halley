#pragma once

#include "halley/core/api/halley_api_internal.h"

namespace Halley {
	class AVFMovieAPI : public MovieAPIInternal {
	public:
		explicit AVFMovieAPI(SystemAPI& system);
		void init() override;
		void deInit() override;

		bool canPlayVideo() const override { return true; }
		std::shared_ptr<MoviePlayer> makePlayer(VideoAPI& video, AudioAPI& audio, std::shared_ptr<ResourceDataStream> data) override;

	private:
		SystemAPI& system;
	};
}
