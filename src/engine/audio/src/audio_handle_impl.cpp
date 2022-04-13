#include "audio_handle_impl.h"
#include "audio_facade.h"
#include "audio_engine.h"
#include <algorithm>
#include "behaviours/audio_voice_fade_behaviour.h"
#include "halley/support/logger.h"

using namespace Halley;

AudioHandleImpl::AudioHandleImpl(AudioFacade& facade, AudioEventId eventId, AudioEmitterId emitterId)
	: facade(facade)
	, eventId(eventId)
	, emitterId(emitterId)
{
}

void AudioHandleImpl::setGain(float gain)
{
	if (std::abs(gain - this->gain) > 0.00001f) {
		this->gain = gain;
		enqueueForVoices([gain] (AudioVoice& src)
		{
			src.setUserGain(gain);
		});
	}
}

void AudioHandleImpl::setVolume(float volume)
{
	setGain(volumeToGain(volume));
}

void AudioHandleImpl::setPosition(Vector2f pos)
{
	enqueue([pos] (AudioEmitter& src)
	{
		src.setPosition(AudioPosition::makePositional(Vector3f(pos)));
	});
}

void AudioHandleImpl::setPan(float pan)
{
	enqueue([pan] (AudioEmitter& src)
	{
		src.setPosition(AudioPosition::makeUI(pan));
	});
}

void AudioHandleImpl::stop(float fadeTime)
{
	enqueueForVoices([fadeTime] (AudioVoice& src)
	{
		if (fadeTime >= 0.001f) {
			src.addBehaviour(std::make_unique<AudioVoiceFadeBehaviour>(fadeTime, 1.0f, 0.0f, true));
		} else {
			src.stop();
		}
	});
}

// This is kind of like a unique_ptr, but copying it also moves it. >_>
template <typename T>
class BadPointer {
public:
	BadPointer(T* v) : v(v) {}
	BadPointer(const BadPointer& other) : v(other.v) { other.v = nullptr; }
	BadPointer(BadPointer&& other) : v(other.v) { other.v = nullptr; }
	~BadPointer() { delete v; }

	T* release()
	{
		auto value = v;
		v = nullptr;
		return value;
	}

private:
	mutable T* v = nullptr;
};

void AudioHandleImpl::addBehaviour(std::unique_ptr<AudioVoiceBehaviour> b)
{
	// Gotta work around std::function requiring copyable
	BadPointer<AudioVoiceBehaviour> behaviour = b.release();
	enqueueForVoices([behaviour] (AudioVoice& src) mutable
	{
		auto b = std::unique_ptr<AudioVoiceBehaviour>(behaviour.release());
		if (b) {
			src.addBehaviour(std::move(b));
		} else {
			Logger::logWarning("AudioVoiceBehaviour lost since event has more than one voice.");
		}
	});
}

bool AudioHandleImpl::isPlaying() const
{
	auto& playing = facade.playingSounds;
	return std::binary_search(playing.begin(), playing.end(), eventId);
}


void AudioHandleImpl::enqueue(std::function<void(AudioEmitter& src)> f)
{
	auto id = emitterId;
	AudioEngine* engine = facade.engine.get();
	facade.enqueue([id, engine, f = std::move(f)] () {
		auto* emitter = engine->getEmitter(id);
		if (emitter) {
			f(*emitter);
		}
	});
}

void AudioHandleImpl::enqueueForVoices(std::function<void(AudioVoice& src)> f)
{
	auto id = emitterId;
	auto evId = eventId;
	AudioEngine* engine = facade.engine.get();
	facade.enqueue([id, evId, engine, f = std::move(f)] () {
		auto* emitter = engine->getEmitter(id);
		if (emitter) {
			for (auto& v: emitter->getVoices()) {
				if (v->getEventId() == evId) {
					f(*v);
				}
			}
		}
	});
}
