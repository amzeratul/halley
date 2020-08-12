#pragma once
#include <memory>

namespace Halley {
	class AudioVoice;

	class AudioVoiceBehaviour {
	public:
		virtual ~AudioVoiceBehaviour();
		virtual void onAttach(AudioVoice& audioSource);
	    virtual bool update(float elapsedTime, AudioVoice& audioSource) = 0;

		bool updateChain(float elapsedTime, AudioVoice& audioSource);
		void addToChain(std::unique_ptr<AudioVoiceBehaviour> next);
		std::unique_ptr<AudioVoiceBehaviour> releaseNext();

	private:
		std::unique_ptr<AudioVoiceBehaviour> next;
    };

	class AudioVoiceFadeBehaviour final : public AudioVoiceBehaviour {
	public:
		AudioVoiceFadeBehaviour(float fadeTime, float sourceVolume, float targetVolume, bool stopAtEnd);

		void onAttach(AudioVoice& audioSource) override;
		bool update(float elapsedTime, AudioVoice& audioSource) override;

	private:
		float curTime;
		float fadeTime;
		float volume0;
		float volume1;
		bool stopAtEnd;
	};
}
