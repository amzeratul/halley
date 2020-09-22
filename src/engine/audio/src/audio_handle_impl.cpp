#include "audio_handle_impl.h"
#include "audio_facade.h"
#include "audio_engine.h"
#include <algorithm>
#include "behaviours/audio_voice_fade_behaviour.h"

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
		src.setUserGain(gain);
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
	enqueue([behaviour] (AudioVoice& src) mutable
	{
		src.addBehaviour(std::unique_ptr<AudioVoiceBehaviour>(behaviour.release()));
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
