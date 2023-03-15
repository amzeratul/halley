#include "halley/audio/audio_fade.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/data_structures/config_node.h"
using namespace Halley;

AudioFade::AudioFade(float length, AudioFadeCurve curve)
	: length(length)
	, curve(curve)
{
}

AudioFade::AudioFade(float length, float delay, AudioFadeCurve curve)
	: length(length)
	, delay(delay)
	, curve(curve)
{
}

AudioFade::AudioFade(const ConfigNode& node)
{
	length = node["length"].asFloat(0.0f);
	delay = node["delay"].asFloat(0.0f);
	curve = fromString<AudioFadeCurve>(node["curve"].asString("none"));
}

ConfigNode AudioFade::toConfigNode() const
{
	ConfigNode::MapType result;
	if (length > 0.0f) {
		result["length"] = length;
	}
	if (delay > 0.0f) {
		result["delay"] = delay;
	}
	if (curve != AudioFadeCurve::None) {
		result["curve"] = toString(curve);
	}
	return result;
}

float AudioFade::evaluate(float time) const
{
	switch (curve) {
	case AudioFadeCurve::None:
		return 1.0f;
	case AudioFadeCurve::Linear:
		return clamp(time / length, 0.0f, 1.0f);
	case AudioFadeCurve::Sinusoidal:
		return smoothCos(clamp(time / length, 0.0f, 1.0f));
	case AudioFadeCurve::Sqrt:
		return std::sqrt(clamp(time / length, 0.0f, 1.0f));
	case AudioFadeCurve::Sine:
		return std::sin(clamp(time / length, 0.0f, 1.0f) * pif() * 0.5f);
	}
	return 1.0f;
}

float AudioFade::getLength() const
{
	return length;
}

void AudioFade::setLength(float len)
{
	length = len;
}

float AudioFade::getDelay() const
{
	return delay;
}

void AudioFade::setDelay(float value)
{
	delay = value;
}

AudioFadeCurve AudioFade::getCurve() const
{
	return curve;
}

void AudioFade::setCurve(AudioFadeCurve c)
{
	curve = c;
}

bool AudioFade::hasFade() const
{
	return curve != AudioFadeCurve::None && (length > 0.0f || delay > 0.0f);
}

void AudioFade::serialize(Serializer& s) const
{
	s << length;
	s << delay;
	s << static_cast<int>(curve);
}

void AudioFade::deserialize(Deserializer& s)
{
	s >> length;
	s >> delay;
	int c;
	s >> c;
	curve = static_cast<AudioFadeCurve>(c);
}

void AudioFader::startFade(float from, float to, const AudioFade& fade)
{
	this->fade = fade;
	startVal = from;
	endVal = to;
	time = -fade.getDelay();

	const float delta = std::abs(from - to);
	if (delta < 0.001f) {
		fading = false;
		timeScale = 1;
	} else {
		fading = true;
		timeScale = 1.0f / delta;
	}
}

void AudioFader::stopAndSetValue(float value)
{
	fading = false;
	startVal = endVal = value;
}

bool AudioFader::update(float t)
{
	if (fading) {
		time += t * timeScale;
		if (time >= fade.getLength()) {
			time = fade.getLength();
			fading = false;
			return true;
		}
	}

	return false;
}

bool AudioFader::isFading() const
{
	return fading;
}

float AudioFader::getCurrentValue() const
{
	return fading ? lerp(startVal, endVal, fade.evaluate(time)) : endVal;
}

float AudioFader::getTargetValue() const
{
	return endVal;
}
