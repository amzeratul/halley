#pragma once
#include "api/halley_api_internal.h"

namespace Halley
{
	class DummyMovieAPI : public MovieAPIInternal
	{
		void init() override;
		void deInit() override;
	};
}
