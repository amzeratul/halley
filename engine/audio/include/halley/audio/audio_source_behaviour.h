#pragma once

namespace Halley {
	class AudioSource;

	class AudioSourceBehaviour {
	public:
		virtual ~AudioSourceBehaviour();
		virtual void onAttach(AudioSource& audioSource);
	    virtual bool update(float elapsedTime, AudioSource& audioSource) = 0;
    };

	class AudioSourceFadeBehaviour: public AudioSourceBehaviour	{
	public:
		AudioSourceFadeBehaviour(float fadeTime, float targetGain, bool stopAtEnd);

		void onAttach(AudioSource& audioSource) override;
		bool update(float elapsedTime, AudioSource& audioSource) override;

	private:
		float curTime;
		float fadeTime;
		float gain0;
		float gain1;
		bool stopAtEnd;
	};
}
