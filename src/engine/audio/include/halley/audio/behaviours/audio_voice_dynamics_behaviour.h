#include "audio_voice_behaviour.h"

namespace Halley {
	class AudioEngine;
	class AudioDynamicsConfig;

	class AudioVoiceDynamicsBehaviour final : public AudioVoiceBehaviour {
	public:
		AudioVoiceDynamicsBehaviour(const AudioDynamicsConfig& config, AudioEngine& engine);

		void onAttach(AudioVoice& audioSource) override;
		bool update(float elapsedTime, AudioVoice& audioSource) override;

	private:
		const AudioDynamicsConfig& config;
		AudioEngine& engine;
	};
}
