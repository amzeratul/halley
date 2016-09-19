#pragma once
#include <vector>
#include "halley/core/api/audio_api.h"

namespace Halley
{
	struct AudioBuffer
	{
		std::vector<AudioSamplePack> samples;
	};
}
