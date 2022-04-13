#include "audio_emitter_handle_impl.h"

#include "audio_engine.h"
#include "audio_facade.h"

using namespace Halley;

AudioEmitterHandleImpl::AudioEmitterHandleImpl(AudioFacade& facade, AudioEmitterId id)
	: facade(facade)
	, id(id)
{
}

AudioEmitterHandleImpl::~AudioEmitterHandleImpl()
{
	AudioEngine* engine = facade.engine.get();
	const auto emId = id;

	facade.enqueue([=] ()
	{
		if (detached) {
			auto* e = engine->getEmitter(emId);
			if (e) {
				e->makeTemporary();
			}
		} else {
			engine->destroyEmitter(emId);
		}
	});
}

AudioEmitterId AudioEmitterHandleImpl::getId() const
{
	return id;
}

void AudioEmitterHandleImpl::detach()
{
	detached = true;
}
