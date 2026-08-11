#include <systems/audio_system.h>

#include <algorithm>

using namespace Halley;

class AudioSystem final : public AudioSystemBase<AudioSystem>, public IAudioSystemInterface {
public:
	void preInit()
	{
		getWorld().setInterface(static_cast<IAudioSystemInterface*>(this));
	}

	void init()
	{
		if (auto* devService = tryGetDevService()) {
			initConsoleCommands(*devService);

			if (devService->isDevMode()) {
				devService->loadAudioEventLogging(*getAPI().audio);
			}
		}
	}

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
		if (msg.emitter.isValid()) {
			playAudio(msg.event, msg.emitter);
		}
	}

	void playAudio(const String& eventId, EntityId entityId) override
	{
		if (const auto* source = sourceFamily.tryFind(entityId)) {
			getAPI().audio->postEvent(eventId, source->audioSource.emitter);
			return;
		}

		// Not a source
		auto e = getWorld().tryGetEntity(entityId);
		if (e.isValid()) {
			if (const auto* transform2d = e.tryGetComponent<Transform2DComponent>(entityId)) {
				auto pos = transform2d->getWorldPosition();
				playAudio(eventId, pos, findRegion(pos));
				return;
			}
		}

		// Fallback
		getAPI().audio->postEvent(eventId);
	}

	void playAudio(const String& event, WorldPosition position, std::optional<AudioRegionId> regionId) override
	{
		auto emitter = getAPI().audio->createEmitter(AudioPosition::makePositional(position.pos));
		if (regionId) {
			emitter->setRegion(*regionId);
		} else {
			emitter->setRegion(findRegion(position).value_or(0));
		}
		emitter->detach();
		getAPI().audio->postEvent(event, emitter);
	}

	void setVariable(EntityId entityId, const String& variableName, float value) override
	{
		if (const auto* source = sourceFamily.tryFind(entityId)) {
			source->audioSource.emitter->setVariable(variableName, value);
		}
	}

	void setGlobalVariable(const String& variableName, float value) override
	{
		getAPI().audio->getGlobalEmitter()->setVariable(variableName, value);
	}

	void setSwitch(EntityId entityId, const String& switchName, const String& value) override
	{
		if (const auto* source = sourceFamily.tryFind(entityId)) {
			source->audioSource.emitter->setSwitch(switchName, value);
		}
	}

	void setGlobalSwitch(const String& switchName, const String& value) override
	{
		getAPI().audio->getGlobalEmitter()->setSwitch(switchName, value);
	}

	std::optional<String> getSourceName(AudioEmitterId id) const override
	{
		for (const auto& source: sourceFamily) {
			if (source.audioSource.emitter && source.audioSource.emitter->getId() == id) {
				return getWorld().getEntity(source.entityId).getName();
			}
		}

		for (auto& f: emitterNameLookups) {
			if (auto value = f(id)) {
				return *value;
			}
		}

		return std::nullopt;
	}

	String getRegionName(AudioRegionId id) const override
	{
		return getAPI().audio->getRegionName(id);
	}

	std::optional<AudioRegionId> findRegion(WorldPosition pos)
	{
		if (regionLookup) {
			return regionLookup(pos);
		}
		return std::nullopt;
	}

	void setRegionLookup(std::function<AudioRegionId(WorldPosition pos)> f) override
	{
		regionLookup = f;
	}

	void addEmitterNameLookup(std::function<std::optional<String>(AudioEmitterId)> f) override
	{
		emitterNameLookups += std::move(f);
	}

private:
	String curRegionId;
	String curRegionPreset;
	String curFloorType;
	std::function<AudioRegionId(WorldPosition pos)> regionLookup;
	Vector<std::function<std::optional<String>(AudioEmitterId)>> emitterNameLookups;
	
	void updateListeners(Time t)
	{
		t = std::max(t, 0.00001);

		AudioAPI& audio = *getAPI().audio;
		for (auto& listener: listenerFamily) {
			const auto pos = Vector3f(listener.transform2D.getGlobalPosition());
			const auto lastPos = listener.audioListener.lastPos;
			const auto deltaPos = pos - lastPos;
			const auto vel = deltaPos.length() < 15.0f ? deltaPos / static_cast<float>(t) : Vector3f();
			listener.audioListener.velAverage.add(vel);
			listener.audioListener.lastPos = pos;
			audio.setListener(AudioListenerData(lastPos, listener.audioListener.velAverage.getMean(), listener.audioListener.referenceDistance, listener.audioListener.speedOfSound, listener.audioListener.regions));
		}
	}

	AudioPosition getAudioPosition(SourceFamily& e, Vector3f vel)
	{
		const auto attenuation = AudioAttenuation(e.audioSource.rangeMin, e.audioSource.rangeMax, e.audioSource.rollOff, e.audioSource.curve);
		const auto pos = e.transform2D.getGlobalPosition() + e.audioSource.offset;

		if (e.audioSource.polygon.isValid()) {
			return AudioPosition::makePositional(pos, e.audioSource.polygon, attenuation, vel.xy());
		} else {
			return AudioPosition::makePositional(Vector3f(pos), attenuation, vel);
		}
	}

	void initSource(SourceFamily& e)
	{
		AudioAPI& audio = *getAPI().audio;
		e.audioSource.emitter = getAPI().audio->createEmitter(getAudioPosition(e, {}));
		if (e.audioSource.event) {
			audio.postEvent(e.audioSource.event.get(), e.audioSource.emitter);
		}
	}

	void deInitSource(SourceFamily& e)
	{
		e.audioSource.emitter = {};
	}

	void updateSources(Time t)
	{
		t = std::max(t, 0.00001);

		for (auto& source: sourceFamily) {
			Vector3f vel;
			const auto pos = Vector3f(source.transform2D.getGlobalPosition());
			const auto lastPos = source.audioSource.lastPos;
			source.audioSource.lastPos = pos;
			source.audioSource.moved = pos != lastPos;

			if (source.velocity) {
				vel = Vector3f(source.velocity->velocity, 0);
			} else if (source.audioSource.canAutoVel) {
				vel = (pos - lastPos) / static_cast<float>(t);
			}

			source.audioSource.emitter->setPosition(getAudioPosition(source, vel));
		}
	}

	void initConsoleCommands(DevService& devService)
	{
		devService.getConsoleCommands().addCommand("audioLogEvents", [=, this] (Vector<String> args) -> String {
			if (auto* audio = getAPI().audio) {
				std::optional<String> prefix;
				if (!args.empty()) {
					prefix = args[0];
				}

				auto* devService = tryGetDevService();

				if (audio->getEventLogging() && !prefix) {
					audio->setEventLogging(std::nullopt);
					if (devService) {
						devService->setAudioEventLogging(std::nullopt);
					}
					return "Audio log disabled.";
				} else {
					audio->setEventLogging(LoggerLevel::Dev, prefix);
					if (devService) {
						devService->setAudioEventLogging(LoggerLevel::Dev, prefix);
					}
					return prefix ? "Audio log enabled for " + *prefix + "." : "Audio log enabled.";
				}
			} else {
				return "No audio subsystem.";
			}
		}, { UIDebugConsoleSyntax::Variant(), {{"prefix", "Halley::String "}}});
	}
};

REGISTER_SYSTEM(AudioSystem)
