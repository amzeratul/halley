#include "audio_voice_behaviour.h"

namespace Halley {

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
