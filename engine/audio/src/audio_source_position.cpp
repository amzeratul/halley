#include "audio_source_position.h"

using namespace Halley;


AudioSourcePosition::AudioSourcePosition()
	: isUI(true)
	, isPannable(false)
{
}

AudioSourcePosition::AudioSourcePosition(Vector3f pos, bool isUI, bool isPannable)
	: pos(pos)
	, isUI(isUI)
	, isPannable(isPannable)
{
}

AudioSourcePosition AudioSourcePosition::makeUI(float pan)
{
	return AudioSourcePosition(Vector3f(pan, 0, 0), true, true);
}

AudioSourcePosition AudioSourcePosition::makePositional(Vector3f pos)
{
	return AudioSourcePosition(pos, false, true);
}

AudioSourcePosition AudioSourcePosition::makeFixed()
{
	return AudioSourcePosition(Vector3f(), true, false);
}

static float gain2DPan(float srcPan, float dstPan)
{
	return std::max(0.0f, 1.0f - std::abs(srcPan - dstPan));
}

void AudioSourcePosition::setMix(size_t nSrcChannels, gsl::span<const AudioChannelData> dstChannels, gsl::span<float, 16> dst, float gain) const
{
	const size_t nDstChannels = size_t(dstChannels.size());

	if (isPannable) {
		// Pannable (mono) sounds
		Expects(nSrcChannels == 1);
		if (isUI) {
			for (size_t i = 0; i < nDstChannels; ++i) {
				dst[i] = gain2DPan(pos.x, dstChannels[i].pan) * gain;
			}
		} else {
			// TODO: in-world sounds, do this relative to listener
			for (size_t i = 0; i < nDstChannels; ++i) {
				dst[i] = gain2DPan(0.5f, dstChannels[i].pan) * gain;
			}
		}
	} else {
		// Non-pannable (stereo) sounds
		Expects(nSrcChannels == 2);
		for (size_t i = 0; i < nSrcChannels; ++i) {
			float srcPan = i == 0 ? 0.0f : 1.0f;
			for (size_t j = 0; j < nDstChannels; ++j) {
				dst[i * nSrcChannels + j] = gain2DPan(srcPan, dstChannels[j].pan) * gain;
			}
		}
	}
}
