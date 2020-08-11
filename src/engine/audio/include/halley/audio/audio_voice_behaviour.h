#pragma once

namespace Halley {
	class AudioVoice;

	class AudioVoiceBehaviour {
	public:
		virtual ~AudioVoiceBehaviour();
		virtual void onAttach(AudioVoice& audioSource);
	    virtual bool update(float elapsedTime, AudioVoice& audioSource) = 0;
    };

	class AudioVoiceFadeBehaviour final : public AudioVoiceBehaviour {
	public:
		AudioVoiceFadeBehaviour(float fadeTime, float targetVolume, bool stopAtEnd);

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
