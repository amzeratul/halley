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

	class AudioPosition
	{
	public:
		static AudioPosition makeUI(float pan = 0.0f); // -1.0f = left, 1.0f = right
		static AudioPosition makePositional(Vector2f pos, float referenceDistance = 200.0f, float maxDistance = 400.0f);
		static AudioPosition makePositional(Vector3f pos, float referenceDistance = 200.0f, float maxDistance = 400.0f);
		static AudioPosition makeFixed();

		void setMix(size_t srcChannels, gsl::span<const AudioChannelData> dstChannels, gsl::span<float, 16> dst, float gain, const AudioListenerData& listener) const;

		AudioPosition();
		
	private:
		Vector3f pos;
		float referenceDistance;
		float maxDistance;
		bool isUI;
		bool isPannable;
	};
}
