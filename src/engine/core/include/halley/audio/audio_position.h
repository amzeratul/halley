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
			SpatialSource(Vector2f pos, Vector2f vel, float referenceDistance = 200.0f, float maxDistance = 400.0f);
			SpatialSource(Vector3f pos, Vector3f vel, float referenceDistance = 200.0f, float maxDistance = 400.0f);

			Vector3f pos;
			Vector3f velocity;
			float referenceDistance;
			float maxDistance;
		};

		AudioPosition();

		static AudioPosition makeUI(float pan = 0.0f); // -1.0f = left, 1.0f = right
		static AudioPosition makePositional(Vector2f pos, float referenceDistance = 200.0f, float maxDistance = 400.0f, Vector2f velocity = {});
		static AudioPosition makePositional(Vector3f pos, float referenceDistance = 200.0f, float maxDistance = 400.0f, Vector3f velocity = {});
		static AudioPosition makePositional(Vector<SpatialSource> sources);
		static AudioPosition makeFixed();

		void setMix(size_t srcChannels, gsl::span<const AudioChannelData> dstChannels, gsl::span<float, 16> dst, float gain, const AudioListenerData& listener) const;
		void setPosition(Vector3f position);

		float getDopplerShift(const AudioListenerData& listener) const;

	private:
		Vector<SpatialSource> sources;
		float pan = 0;
		bool isUI = false;
		bool isPannable = false;

		void setMixFixed(size_t srcChannels, gsl::span<const AudioChannelData> dstChannels, gsl::span<float, 16> dst, float gain, const AudioListenerData& listener) const;
		void setMixUI(gsl::span<const AudioChannelData> dstChannels, gsl::span<float, 16> dst, float gain, const AudioListenerData& listener) const;
		void setMixPositional(size_t nSrcChannels, gsl::span<const AudioChannelData> dstChannels, gsl::span<float, 16> dst, float gain, const AudioListenerData& listener) const;
	};
}
