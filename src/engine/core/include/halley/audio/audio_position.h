#pragma once
#include "halley/maths/vector3.h"
#include <gsl/span>

#include "audio_attenuation.h"
#include "halley/maths/polygon.h"

namespace Halley
{
	class AudioListenerData;
	class AudioChannelData;

	class AudioPosition
	{
	public:
		struct SpatialSource {
			SpatialSource() = default;
			[[deprecated]] SpatialSource(Vector2f pos, Vector2f vel, float referenceDistance, float maxDistance);
			[[deprecated]] SpatialSource(Vector3f pos, Vector3f vel, float referenceDistance, float maxDistance);
			SpatialSource(Vector2f pos, Vector2f vel, AudioAttenuation attenuation = {});
			SpatialSource(Vector2f pos, Polygon polygon, Vector2f vel, AudioAttenuation attenuation = {});
			SpatialSource(Vector3f pos, Vector3f vel, AudioAttenuation attenuation = {});

			Vector3f pos;
			Vector3f velocity;
			AudioAttenuation attenuation;
			Polygon polygon;
		};

		AudioPosition();

		static AudioPosition makeUI(float pan = 0.0f); // -1.0f = left, 1.0f = right
		[[deprecated]] static AudioPosition makePositional(Vector2f pos, float referenceDistance, float maxDistance, Vector2f velocity = {});
		[[deprecated]] static AudioPosition makePositional(Vector3f pos, float referenceDistance, float maxDistance, Vector3f velocity = {});
		static AudioPosition makePositional(Vector2f pos, AudioAttenuation attenuation = {}, Vector2f velocity = {});
		static AudioPosition makePositional(Vector2f pos, Polygon polygon, AudioAttenuation attenuation = {}, Vector2f velocity = {});
		static AudioPosition makePositional(Vector3f pos, AudioAttenuation attenuation = {}, Vector3f velocity = {});
		static AudioPosition makePositional(Vector<SpatialSource> sources);
		static AudioPosition makeFixed();

		// Returns distance
		float setMix(size_t srcChannels, gsl::span<const AudioChannelData> dstChannels, gsl::span<float, 16> dst, float gain, const AudioListenerData& listener, const std::optional<AudioAttenuation>& attenuationOverride) const;
		float getDistance(const AudioListenerData& listener) const;

		void setPosition(Vector3f position);
		float getAttenuation(const AudioListenerData& listener, const std::optional<AudioAttenuation>& attenuationOverride) const;

		float getDopplerShift(const AudioListenerData& listener) const;

	private:
		Vector<SpatialSource> sources;
		float uiPan = 0;
		bool isUI = false;
		bool isPannable = false;

		struct AttenuationResult {
			float attenuation;
			float pan;
			float distance;
		};

		void setMixFixed(size_t srcChannels, gsl::span<const AudioChannelData> dstChannels, gsl::span<float, 16> dst, float gain, const AudioListenerData& listener) const;
		void setMixUI(gsl::span<const AudioChannelData> dstChannels, gsl::span<float, 16> dst, float gain, const AudioListenerData& listener) const;
		float setMixPositional(size_t nSrcChannels, gsl::span<const AudioChannelData> dstChannels, gsl::span<float, 16> dst, float gain, const AudioListenerData& listener, const std::optional<AudioAttenuation>& attenuationOverride) const;
		AttenuationResult getAttenuationAndPanPositional(const AudioListenerData& listener, const std::optional<AudioAttenuation>& attenuationOverride) const;
	};
}
