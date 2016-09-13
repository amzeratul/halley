#pragma once
#include "halley/core/api/halley_api_internal.h"

namespace Halley
{
	class AudioSDL final : public AudioAPIInternal
	{
	public:
		void init() override;
		void deInit() override;
	};
}
