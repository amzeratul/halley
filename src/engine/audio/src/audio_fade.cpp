#include "audio_fade.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/data_structures/config_node.h"
using namespace Halley;

AudioFade::AudioFade(float length, AudioFadeCurve curve)
	: length(length)
	, curve(curve)
{
}

AudioFade::AudioFade(const ConfigNode& node)
{
	length = node["length"].asFloat(0.0f);
	curve = fromString<AudioFadeCurve>(node["curve"].asString("linear"));
}

ConfigNode AudioFade::toConfigNode() const
{
	ConfigNode::MapType result;
	if (length > 0.0f) {
		result["length"] = length;
	}
	if (curve != AudioFadeCurve::Linear) {
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
	}
	return 1.0f;
}

float AudioFade::getLength() const
{
	return length;
}

bool AudioFade::hasFade() const
{
	return curve != AudioFadeCurve::None;
}

void AudioFade::serialize(Serializer& s) const
{
	s << length;
	s << static_cast<int>(curve);
}

void AudioFade::deserialize(Deserializer& s)
{
	s >> length;
	int c;
	s >> c;
	curve = static_cast<AudioFadeCurve>(c);
}

void AudioFader::startFade(float from, float to, const AudioFade& fade)
{
	fading = true;
	startVal = from;
	endVal = to;
	time = 0;
	this->fade = fade;
}

void AudioFader::stopAndSetValue(float value)
{
	fading = false;
	startVal = endVal = value;
}

bool AudioFader::update(float t)
{
	if (fading) {
		time += t;
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
