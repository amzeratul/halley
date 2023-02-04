#pragma once
#include "audio_clip.h"

namespace Halley
{
	class AudioClipStreaming final : public IAudioClip
	{
	public:
		AudioClipStreaming(uint8_t numChannels);

		void addInterleavedSamples(AudioSamplesConst src);

		size_t copyChannelData(size_t channelN, size_t pos, size_t len, float gain0, float gain1, AudioSamples dst) const override;
		uint8_t getNumberOfChannels() const override;
		size_t getLength() const override;
		size_t getSamplesLeft() const;

	private:
		size_t length = 0;
		mutable Vector<Vector<AudioSample>> buffers;
		mutable std::mutex mutex;
		uint8_t numChannels = 0;
	};
}
