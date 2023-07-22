#include <systems/audio_system.h>

using namespace Halley;

class AudioSystem final : public AudioSystemBase<AudioSystem> {
public:
	void onEntitiesAdded(Span<SourceFamily> es)
	{
		for (auto& e: es) {
			initSource(e);
		}
	}

	void onEntitiesRemoved(Span<SourceFamily> es)
	{
		for (auto& e: es) {
			deInitSource(e);
		}
	}

	void onEntitiesReloaded(Span<SourceFamily*> es)
	{
		for (auto& e: es) {
			initSource(*e);
		}
	}

	void update(Time t)
	{
		updateListeners(t);
		updateSources(t);
	}

	void onMessageReceived(const PlayNetworkSoundSystemMessage& msg) override
	{
		if (const auto* source = sourceFamily.tryFind(msg.emitter)) {
			getAPI().audio->postEvent(msg.event, source->audioSource.emitter);
		}
	}

private:
	String curRegionId;
	String curRegionPreset;
	String curFloorType;
	
	void updateListeners(Time t)
	{
		if (t < 0.00001) {
			t = 0.00001;
		}

		AudioAPI& audio = *getAPI().audio;
		for (auto& listener: listenerFamily) {
			const auto pos = Vector3f(listener.transform2D.getGlobalPosition());
			const auto lastPos = listener.audioListener.lastPos;
			const auto deltaPos = pos - lastPos;
			const auto vel = deltaPos.length() < 15.0f ? deltaPos / static_cast<float>(t) : Vector3f();
			listener.audioListener.velAverage.add(vel);
			listener.audioListener.lastPos = pos;
			audio.setListener(AudioListenerData(lastPos, listener.audioListener.velAverage.getMean(), listener.audioListener.referenceDistance, listener.audioListener.speedOfSound));
		}
	}

	AudioPosition getAudioPosition(SourceFamily& e, Vector3f vel)
	{
		return AudioPosition::makePositional(Vector3f(e.transform2D.getGlobalPosition()), e.audioSource.rangeMin, e.audioSource.rangeMax, vel);
	}

	void initSource(SourceFamily& e)
	{
		AudioAPI& audio = *getAPI().audio;
		e.audioSource.emitter = getAPI().audio->createEmitter(getAudioPosition(e, {}));
		if (e.audioSource.event) {
			audio.postEvent(*e.audioSource.event, e.audioSource.emitter);
		}
	}

	void deInitSource(SourceFamily& e)
	{
		e.audioSource.emitter = {};
	}

	void updateSources(Time t)
	{
		if (t < 0.00001) {
			t = 0.00001;
		}

		for (auto& source: sourceFamily) {
			Vector3f vel;
			if (source.velocity) {
				vel = Vector3f(source.velocity->velocity, 0);
			} else if (source.audioSource.canAutoVel) {
				const auto pos = Vector3f(source.transform2D.getGlobalPosition());
				const auto lastPos = source.audioSource.lastPos;
				vel = (pos - lastPos) / static_cast<float>(t);
				source.audioSource.lastPos = pos;
			}

			source.audioSource.emitter->setPosition(getAudioPosition(source, vel));
		}
	}
};

REGISTER_SYSTEM(AudioSystem)
