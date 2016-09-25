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
	enqueue([gain] (AudioSource& src)
	{
		src.setGain(gain);
	});
}

void AudioHandleImpl::setPosition(Vector2f pos)
{
	enqueue([pos] (AudioSource& src)
	{
		src.setAudioSourcePosition(AudioSourcePosition::makePositional(Vector3f(pos)));
	});
}

void AudioHandleImpl::setPan(float pan)
{
	enqueue([pan] (AudioSource& src)
	{
		src.setAudioSourcePosition(AudioSourcePosition::makeUI(pan));
	});
}

void AudioHandleImpl::stop()
{
	enqueue([] (AudioSource& src)
	{
		src.stop();
	});
}

bool AudioHandleImpl::isPlaying() const
{
	auto& playing = facade.playingSounds;
	return std::binary_search(playing.begin(), playing.end(), handleId);
}

void AudioHandleImpl::enqueue(std::function<void(AudioSource& src)> f)
{
	size_t id = handleId;
	AudioEngine* engine = facade.engine.get();
	facade.enqueue([id, engine, f] () {
		auto src = engine->getSource(id);
		if (src) {
			f(*src);
		}
	});
}
