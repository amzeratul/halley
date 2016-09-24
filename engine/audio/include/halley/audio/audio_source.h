#pragma once
#include "audio_clip.h"
#include "halley/maths/vector3.h"

namespace Halley {
	class AudioChannelData
	{
	public:
		float pan; // TODO, do this right
	};

	class AudioSourcePosition
	{
	public:
		static AudioSourcePosition makeUI(float pan);
		static AudioSourcePosition makePositional(Vector3f pos);

		void setMix(gsl::span<const AudioChannelData> channels, gsl::span<float, 8> dst, float gain) const;
		
	private:
		AudioSourcePosition(bool isUI, Vector3f pos);

		bool isUI;
		Vector3f pos;
	};

    class AudioSource {
    public:
		AudioSource(std::shared_ptr<AudioClip> clip, AudioSourcePosition sourcePos, float gain);

		void start();
		void stop();

		bool isPlaying() const;
		bool isReady() const;
		bool isDone() const;

		void update(gsl::span<const AudioChannelData> channels);
		void mixToBuffer(size_t srcChannel, size_t dstChannel, gsl::span<AudioSamplePack> tmp, gsl::span<AudioSamplePack> out);
		void advancePlayback(size_t samples);
		
    private:
		std::shared_ptr<AudioClip> clip;
		AudioSourcePosition sourcePos;
		float gain;

		size_t playbackPos = 0;
		size_t playbackLength = 0;
		bool playing = false;
		bool done = false;

		std::array<float, 8> prevChannelMix;
		std::array<float, 8> channelMix;

		void readSourceToBuffer(size_t srcChannel, gsl::span<AudioSamplePack> dst) const;
    };
}
