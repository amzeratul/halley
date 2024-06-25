#include "audio_handle_impl.h"
#include "halley/audio/audio_facade.h"
#include "audio_engine.h"
#include <algorithm>

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

void AudioHandleImpl::setPosition(AudioPosition pos)
{
	enqueue([pos] (AudioEmitter& src)
	{
		src.setPosition(pos);
	});
}

void AudioHandleImpl::setPan(float pan)
{
	enqueue([pan] (AudioEmitter& src)
	{
		src.setPosition(AudioPosition::makeUI(pan));
	});
}

void AudioHandleImpl::play(const AudioFade& audioFade)
{
	enqueueForVoices([fade = audioFade](AudioVoice& src)
	{
		src.play(fade);
	});	
}

void AudioHandleImpl::stop(const AudioFade& audioFade)
{
	enqueueForVoices([fade = audioFade](AudioVoice& src)
	{
		src.stop(fade);
	});
}

void AudioHandleImpl::pause(const AudioFade& audioFade)
{
	enqueueForVoices([fade = audioFade](AudioVoice& src)
	{
		src.pause(fade);
	});
}

void AudioHandleImpl::resume(const AudioFade& audioFade)
{
	enqueueForVoices([fade = audioFade](AudioVoice& src)
	{
		src.resume(fade);
	});
}

void AudioHandleImpl::stop(float fadeTime)
{
	stop(AudioFade(fadeTime, AudioFadeCurve::Linear));
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
