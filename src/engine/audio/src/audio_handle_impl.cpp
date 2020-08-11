#include "audio_handle_impl.h"
#include "audio_facade.h"
#include "audio_engine.h"
#include <algorithm>

using namespace Halley;

AudioHandleImpl::AudioHandleImpl(AudioFacade& facade, uint32_t id)
	: facade(facade)
	, handleId(id)
{
}

void AudioHandleImpl::setGain(float gain)
{
	enqueue([gain] (AudioVoice& src)
	{
		src.setGain(gain);
	});
}

void AudioHandleImpl::setVolume(float volume)
{
	setGain(volumeToGain(volume));
}

void AudioHandleImpl::setPosition(Vector2f pos)
{
	enqueue([pos] (AudioVoice& src)
	{
		src.setAudioSourcePosition(Vector3f(pos));
	});
}

void AudioHandleImpl::setPan(float pan)
{
	enqueue([pan] (AudioVoice& src)
	{
		src.setAudioSourcePosition(AudioPosition::makeUI(pan));
	});
}

void AudioHandleImpl::stop(float fadeTime)
{
	enqueue([fadeTime] (AudioVoice& src)
	{
		if (fadeTime >= 0.001f) {
			src.setBehaviour(std::make_unique<AudioVoiceFadeBehaviour>(fadeTime, 0.0f, true));
		} else {
			src.stop();
		}
	});
}

void AudioHandleImpl::setBehaviour(std::unique_ptr<AudioVoiceBehaviour> b)
{
	// Gotta work around std::function requiring copyable
	const auto behaviour = b.release();
	enqueue([behaviour] (AudioVoice& src) mutable
	{
		src.setBehaviour(std::unique_ptr<AudioVoiceBehaviour>(behaviour));
	});
}

bool AudioHandleImpl::isPlaying() const
{
	auto& playing = facade.playingSounds;
	return std::binary_search(playing.begin(), playing.end(), handleId);
}


void AudioHandleImpl::enqueue(std::function<void(AudioVoice& src)> f)
{
	uint32_t id = handleId;
	AudioEngine* engine = facade.engine.get();
	facade.enqueue([id, engine, f = std::move(f)] () {
		for (const auto& src: engine->getSources(id)) {
			f(*src);
		}
	});
}
