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

void AudioSourcePosition::setMix(gsl::span<const AudioChannelData> channels, gsl::span<float, 8> dst, float gain) const
{
	// TODO
	dst[0] = 0.5f * gain;
	dst[1] = 0.5f * gain;
}
