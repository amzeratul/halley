#include "audio_handle_impl.h"
#include "audio_facade.h"
#include "audio_engine.h"
#include <algorithm>

using namespace Halley;

AudioHandleImpl::AudioHandleImpl(AudioFacade& facade, size_t id)
	: facade(facade)
	, handleId(id)
{
}

void AudioHandleImpl::setGain(float gain)
{
	enqueue([gain] (AudioEmitter& src)
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
	enqueue([pos] (AudioEmitter& src)
	{
		src.setAudioSourcePosition(AudioPosition::makePositional(Vector3f(pos)));
	});
}

void AudioHandleImpl::setPan(float pan)
{
	enqueue([pan] (AudioEmitter& src)
	{
		src.setAudioSourcePosition(AudioPosition::makeUI(pan));
	});
}

void AudioHandleImpl::stop(float fadeTime)
{
	enqueue([fadeTime] (AudioEmitter& src)
	{
		if (fadeTime >= 0.001f) {
			src.setBehaviour(std::make_unique<AudioEmitterFadeBehaviour>(fadeTime, 0.0f, true));
		} else {
			src.stop();
		}
	});
}

void AudioHandleImpl::setBehaviour(std::unique_ptr<AudioEmitterBehaviour> b)
{
	std::shared_ptr<AudioEmitterBehaviour> behaviour = std::move(b);
	enqueue([behaviour] (AudioEmitter& src) mutable
	{
		src.setBehaviour(behaviour);
	});
}

bool AudioHandleImpl::isPlaying() const
{
	auto& playing = facade.playingSounds;
	return std::binary_search(playing.begin(), playing.end(), handleId);
}


void AudioHandleImpl::enqueue(std::function<void(AudioEmitter& src)> f)
{
	size_t id = handleId;
	AudioEngine* engine = facade.engine.get();
	facade.enqueue([id, engine, f] () {
		for (auto& src: engine->getSources(id)) {
			f(*src);
		}
	});
}
