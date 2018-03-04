#pragma once
#include "api/halley_api_internal.h"

namespace Halley
{
	class DummyMovieAPI : public MovieAPIInternal
	{
	public:
		void init() override;
		void deInit() override;

		std::shared_ptr<MoviePlayer> makePlayer(std::shared_ptr<ResourceDataStream> data) override;
	};
}
