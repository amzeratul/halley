#pragma once
#include "halley/maths/vector3.h"
#include <gsl/span>

namespace Halley
{
	class AudioListenerData;

	class AudioChannelData
	{
	public:
		float pan; // TODO, do this right
	};

	class AudioSourcePosition
	{
	public:
		static AudioSourcePosition makeUI(float pan);
		static AudioSourcePosition makePositional(Vector3f pos);
		static AudioSourcePosition makeFixed();

		void setMix(size_t srcChannels, gsl::span<const AudioChannelData> dstChannels, gsl::span<float, 16> dst, float gain, const AudioListenerData& listener) const;

		AudioSourcePosition();
		
	private:
		AudioSourcePosition(Vector3f pos, bool isUI, bool isPannable);

		Vector3f pos;
		bool isUI;
		bool isPannable;
	};
}
