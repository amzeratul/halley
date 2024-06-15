#pragma once

#include <gsl/span>
#include <array>

#include "audio_buffer.h"

namespace Halley
{
    class AudioBufferPool;
    class Random;
    
	class AudioEnv {
	public:
		virtual ~AudioEnv() {}

		virtual Random& getRNG() = 0;
        virtual AudioBufferPool& getPool() const = 0;
	};
}
