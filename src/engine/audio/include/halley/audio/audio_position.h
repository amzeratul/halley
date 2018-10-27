#pragma once
#include "halley/maths/vector3.h"
#include <gsl/span>

namespace Halley
{
	class AudioListenerData;
	class AudioChannelData;

	class AudioPosition
	{
	public:
		struct SpatialSource {
			SpatialSource();
			SpatialSource(Vector2f pos, float referenceDistance = 200.0f, float maxDistance = 400.0f);
			SpatialSource(Vector3f pos, float referenceDistance = 200.0f, float maxDistance = 400.0f);

			Vector3f pos;
			float referenceDistance;
			float maxDistance;
		};

		AudioPosition();

		static AudioPosition makeUI(float pan = 0.0f); // -1.0f = left, 1.0f = right
		static AudioPosition makePositional(Vector2f pos, float referenceDistance = 200.0f, float maxDistance = 400.0f);
		static AudioPosition makePositional(Vector3f pos, float referenceDistance = 200.0f, float maxDistance = 400.0f);
		static AudioPosition makePositional(std::vector<SpatialSource> sources);
		static AudioPosition makeFixed();

		void setMix(size_t srcChannels, gsl::span<const AudioChannelData> dstChannels, gsl::span<float, 16> dst, float gain, const AudioListenerData& listener) const;
		void setPosition(Vector3f position);

	private:
		std::vector<SpatialSource> sources;
		float pan = 0;
		bool isUI = false;
		bool isPannable = false;

		void setMixFixed(size_t srcChannels, gsl::span<const AudioChannelData> dstChannels, gsl::span<float, 16> dst, float gain, const AudioListenerData& listener) const;
		void setMixUI(gsl::span<const AudioChannelData> dstChannels, gsl::span<float, 16> dst, float gain, const AudioListenerData& listener) const;
		void setMixPositional(gsl::span<const AudioChannelData> dstChannels, gsl::span<float, 16> dst, float gain, const AudioListenerData& listener) const;
	};
}
