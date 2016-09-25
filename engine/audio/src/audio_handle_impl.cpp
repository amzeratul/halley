#include "audio_handle_impl.h"
#include "audio_facade.h"
#include "audio_engine.h"

using namespace Halley;

AudioHandleImpl::AudioHandleImpl(AudioFacade& facade, size_t id)
	: facade(facade)
	, handleId(id)
{
}

void AudioHandleImpl::setGain(float gain)
{
	// TODO
}

void AudioHandleImpl::setPosition(Vector2f pos)
{
	// TODO
}

void AudioHandleImpl::setPan(float pan)
{
	// TODO
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
	// TODO
	return false;
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
