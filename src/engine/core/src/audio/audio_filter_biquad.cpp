#include "halley/audio/audio_filter_biquad.h"

#include "halley/audio/audio_buffer.h"

using namespace Halley;

// Some good references:
//
// https://arachnoid.com/BiQuadDesigner/index.html
// https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html

void AudioFilterBiquad::setParameters(float a0, float a1, float a2, float b0, float b1, float b2)
{
	setParameters(a1 / a0, a2 / a0, b0 / a0, b1 / a0, b2 / a0);
}

void AudioFilterBiquad::setParameters(float a1, float a2, float b0, float b1, float b2)
{
	this->a1 = a1;
	this->a2 = a2;
	this->b0 = b0;
	this->b1 = b1;
	this->b2 = b2;
}

void AudioFilterBiquad::setLowPass(float cutoffHz, float sampleRate)
{
	const float Q = 1 / std::sqrtf(2); // Butterworth filter
	const float w0 = 2 * pif() * cutoffHz / sampleRate;
	const float alpha = std::sin(w0) / (2 * Q);
	const float cosw0 = std::cos(w0);

	setParameters(1 + alpha, -2 * cosw0, 1 - alpha, (1 - cosw0) / 2, 1 - cosw0, (1 - cosw0) / 2);
}

float AudioFilterBiquad::processSample(float x, size_t channelNumber)
{
	auto& cn = channels[channelNumber];

    float y = b0 * x + b1 * cn.x1 + b2 * cn.x2 - a1 * cn.y1 - a2 * cn.y2;
    cn.x2 = cn.x1;
    cn.x1 = x;
    cn.y2 = cn.y1;
    cn.y1 = y;

	return y;
}

void AudioFilterBiquad::processSamples(AudioBuffersRef& buffers)
{
	size_t i = 0;
	for (auto* buffer: buffers.getBuffers()) {
		processSamples(*buffer, i++);
	}
}

void AudioFilterBiquad::processSamples(AudioBuffer& buffer, size_t channelNumber)
{
	for (auto& sample: buffer.samples.span()) {
		sample = processSample(sample, channelNumber);
	}
}
