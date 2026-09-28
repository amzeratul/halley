#pragma once
#include "halley/audio/audio_clip.h"
#include "halley/audio/audio_source.h"
#include "halley/maths/range.h"
#include <atomic>

namespace Halley
{
	class AudioSourceClip final : public AudioSource
	{
	public:
		AudioSourceClip(AudioEngine& engine, std::shared_ptr<const IAudioClip> clip, bool looping, float gain, int64_t loopStart, int64_t loopEnd, bool randomiseStart);

		String getName() const override;
		uint8_t getNumberOfChannels() const override;
		bool getAudioData(size_t numSamples, AudioMultiChannelSamples dst) override;
		bool isReady() const override;
		size_t getSamplesLeft() const override;
		void restart() override;
		bool isLooping() override;
		void prime() override;

	private:
		AudioEngine& engine;
		const std::shared_ptr<const IAudioClip> clip;

		struct PlayStream {
			size_t playbackPos = 0;
			size_t endPos = 0;
			bool active = false;
			bool loop = false;
			bool kickOffSecondStream = false;
			std::unique_ptr<IAudioClipStreamHandle> streamingHandle;
		};
		std::array<PlayStream, 2> streams;
		
		// Opening a stream reads its headers and scans to the end of the file, and a random
		// start then bisects to its sample. Both are far too slow for the audio thread (on an HDD like XB1), 
		// so the handle is opened on the disk IO pool. Until it lands, this source plays silence but keeps its
		// position moving, then decodes forward to catch up, so synchronised layers stay aligned. 
		struct PendingHandle {
			std::unique_ptr<IAudioClipStreamHandle> handle;
			size_t startPos = 0;
			std::atomic<bool> ready = false;
		};
		std::shared_ptr<PendingHandle> pendingHandle;
		
		size_t getPrimaryEndPos() const;
		size_t pickStartPos() const;
		void requestStreamHandle(size_t startPos);
		void takeStreamHandle();

		int64_t loopStart = 0;
		int64_t loopEnd = 0;
		float gain = 1;
		float prevGain = 1;

		bool initialised = false;
		bool looping = false;
		bool randomiseStart = false;
	};
}
