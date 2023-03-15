#pragma once
#include "halley/audio/audio_fade.h"
#include "halley/audio/audio_source.h"
#include "halley/audio/sub_objects/audio_sub_object_sequence.h"

namespace Halley
{
	class AudioSourceSequence final : public AudioSource
	{
	public:
		AudioSourceSequence(AudioEngine& engine, AudioEmitter& emitter, const AudioSubObjectSequence& sequenceConfig);

		uint8_t getNumberOfChannels() const override;
		bool getAudioData(size_t samplesRequested, AudioMultiChannelSamples dst) override;
		bool isReady() const override;
		size_t getSamplesLeft() const override;
		void restart() override;

	private:
		enum class TrackState {
			Playing,
			TransitioningOut,
			Done
		};

		struct PlayingTrack {
			std::unique_ptr<AudioSource> source;
			size_t endSamplesOverlap = 0;
			TrackState state = TrackState::Playing;
			float prevGain = 1.0f;
			AudioFader fader;
			bool initialized = false;
			int endSample = 0;

			PlayingTrack() = default;
			PlayingTrack(std::unique_ptr<AudioSource> source)
				: source(std::move(source))
			{}

			size_t getSamplesBeforeNextEvent(size_t fadeLen) const;
			size_t getSamplesBeforeTransition(size_t fadeLen) const;
			size_t getSamplesBeforeEnd() const;
			void initialize();
		};

		AudioEngine& engine;
		AudioEmitter& emitter;
		const AudioSubObjectSequence& sequenceConfig;

		Vector<size_t> playList;
		size_t curTrack = 0;

		bool initialized = false;

		Vector<PlayingTrack> playingTracks;

		void initialize();
		void nextTrack();
		void loadCurrentTrack();
	};
}
