#include "audio_voice_behaviour.h"

namespace Halley {

	class AudioVoiceDynamicsBehaviour final : public AudioVoiceBehaviour {
	public:
		AudioVoiceDynamicsBehaviour();

		void onAttach(AudioVoice& audioSource) override;
		bool update(float elapsedTime, AudioVoice& audioSource) override;
	};
}
