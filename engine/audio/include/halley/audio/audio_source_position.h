#pragma once
#include "halley/maths/vector3.h"
#include <gsl/span>

namespace Halley
{
	
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

		void setMix(gsl::span<const AudioChannelData> channels, gsl::span<float, 8> dst, float gain) const;

		AudioSourcePosition();
		
	private:
		AudioSourcePosition(Vector3f pos, bool isUI, bool isPannable);

		Vector3f pos;
		bool isUI;
		bool isPannable;
	};
}
