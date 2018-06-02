#pragma once

namespace Halley {
	class AudioEmitter;

	class AudioEmitterBehaviour {
	public:
		virtual ~AudioEmitterBehaviour();
		virtual void onAttach(AudioEmitter& audioSource);
	    virtual bool update(float elapsedTime, AudioEmitter& audioSource) = 0;
    };

	class AudioEmitterFadeBehaviour: public AudioEmitterBehaviour {
	public:
		AudioEmitterFadeBehaviour(float fadeTime, float targetVolume, bool stopAtEnd);

		void onAttach(AudioEmitter& audioSource) override;
		bool update(float elapsedTime, AudioEmitter& audioSource) override;

	private:
		float curTime;
		float fadeTime;
		float gain0;
		float gain1;
		bool stopAtEnd;
	};
}
