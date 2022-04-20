#include "audio_emitter_handle_impl.h"

#include "audio_engine.h"
#include "audio_facade.h"

using namespace Halley;

AudioEmitterHandleImpl::AudioEmitterHandleImpl(AudioFacade& facade, AudioEmitterId id, bool owning)
	: facade(facade)
	, id(id)
	, owning(owning)
{
}

AudioEmitterHandleImpl::~AudioEmitterHandleImpl()
{
	AudioEngine* engine = facade.engine.get();
	const auto emId = id;
	const bool isDetached = detached;

	if (owning) {
		facade.enqueue([=] ()
		{
			if (isDetached) {
				auto* e = engine->getEmitter(emId);
				if (e) {
					e->makeTemporary();
				}
			} else {
				engine->destroyEmitter(emId);
			}
		});
	}
}

AudioEmitterId AudioEmitterHandleImpl::getId() const
{
	return id;
}

void AudioEmitterHandleImpl::detach()
{
	detached = true;
}

void AudioEmitterHandleImpl::setSwitch(String switchId, String value)
{
	AudioEngine* engine = facade.engine.get();
	const auto emId = id;

	facade.enqueue([=] () mutable
	{
		auto* em = engine->getEmitter(emId);
		if (em) {
			em->setSwitchValue(switchId, std::move(value));
		}
	});
}

void AudioEmitterHandleImpl::setVariable(String variableId, float value)
{
	AudioEngine* engine = facade.engine.get();
	const auto emId = id;

	facade.enqueue([=] ()
	{
		auto* em = engine->getEmitter(emId);
		if (em) {
			em->setVariableValue(variableId, value);
		}
	});
}

void AudioEmitterHandleImpl::setPosition(AudioPosition position)
{
	AudioEngine* engine = facade.engine.get();
	const auto emId = id;

	facade.enqueue([=] ()
	{
		auto* em = engine->getEmitter(emId);
		if (em) {
			em->setPosition(position);
		}
	});
}

void AudioEmitterHandleImpl::setGain(float gain)
{
	AudioEngine* engine = facade.engine.get();
	const auto emId = id;

	facade.enqueue([=] ()
	{
		auto* em = engine->getEmitter(emId);
		if (em) {
			em->forVoices(0, [&] (AudioVoice& voice)
			{
				voice.setUserGain(gain);
			});
		}
	});
}
