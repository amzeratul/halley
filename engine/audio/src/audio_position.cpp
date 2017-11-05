#include "audio_position.h"
#include "halley/core/api/audio_api.h"

using namespace Halley;


AudioPosition::AudioPosition()
	: isUI(true)
	, isPannable(false)
{
}

AudioPosition AudioPosition::makeUI(float pan)
{
	auto result = AudioPosition();
	result.pos = Vector3f(pan, 0, 0);
	result.isUI = true;
	result.isPannable = true;
	return result;
}

AudioPosition AudioPosition::makePositional(Vector2f pos, float referenceDistance, float maxDistance)
{
	return makePositional(Vector3f(pos), referenceDistance, maxDistance);
}

AudioPosition AudioPosition::makePositional(Vector3f pos, float referenceDistance, float maxDistance)
{
	auto result = AudioPosition();
	result.pos = pos;
	result.isUI = false;
	result.isPannable = true;
	result.referenceDistance = std::max(0.1f, referenceDistance);
	result.maxDistance = std::max(result.referenceDistance, maxDistance);
	return result;
}

AudioPosition AudioPosition::makeFixed()
{
	auto result = AudioPosition();
	result.isUI = true;
	result.isPannable = false;
	return result;
}

static float gain2DPan(float srcPan, float dstPan)
{
	constexpr float piOverTwo = 3.1415926535897932384626433832795f / 2.0f;
	return std::sin(std::max(0.0f, 1.0f - 0.5f * std::abs(srcPan - dstPan)) * piOverTwo);
}

void AudioPosition::setMix(size_t nSrcChannels, gsl::span<const AudioChannelData> dstChannels, gsl::span<float, 16> dst, float gain, const AudioListenerData& listener) const
{
	const size_t nDstChannels = size_t(dstChannels.size());

	if (isPannable) {
		// Pannable (mono) sounds
		Expects(nSrcChannels == 1);
		if (isUI) {
			// UI sound
			for (size_t i = 0; i < nDstChannels; ++i) {
				dst[i] = gain2DPan(pos.x, dstChannels[i].pan) * gain;
			}
		} else {
			// In-world sound
			auto delta = pos - listener.position;
			float pan = clamp(delta.x * 0.01f, 0.0f, 1.0f);
			float len = delta.length();

			float rolloff = 1.0f - clamp((len - referenceDistance) / (maxDistance - referenceDistance), 0.0f, 1.0f);

			for (size_t i = 0; i < nDstChannels; ++i) {
				dst[i] = gain2DPan(pan, dstChannels[i].pan) * gain * rolloff;
			}
		}
	} else {
		// Non-pannable (stereo) sounds
		Expects(nSrcChannels == 2);
		for (size_t i = 0; i < nSrcChannels; ++i) {
			float srcPan = i == 0 ? -1.0f : 1.0f;
			for (size_t j = 0; j < nDstChannels; ++j) {
				dst[i * nSrcChannels + j] = gain2DPan(srcPan, dstChannels[j].pan) * gain;
			}
		}
	}
}
