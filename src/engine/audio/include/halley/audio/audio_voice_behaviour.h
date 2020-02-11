#pragma once

namespace Halley {
	class AudioVoice;

	class AudioVoiceBehaviour {
	public:
		virtual ~AudioVoiceBehaviour();
		virtual void onAttach(AudioVoice& audioSource);
	    virtual bool update(float elapsedTime, AudioVoice& audioSource) = 0;
    };

	class AudioVoiceFadeBehaviour: public AudioVoiceBehaviour {
	public:
		AudioVoiceFadeBehaviour(float fadeTime, float targetVolume, bool stopAtEnd);

		void onAttach(AudioVoice& audioSource) override;
		bool update(float elapsedTime, AudioVoice& audioSource) override;

	private:
		float curTime;
		float fadeTime;
		float gain0;
		float gain1;
		bool stopAtEnd;
	};
}
