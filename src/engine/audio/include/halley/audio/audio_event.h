#pragma once
#include "halley/resources/resource.h"
#include "halley/maths/range.h"
#include "halley/data_structures/maybe.h"
#include "audio_clip.h"
#include "audio_dynamics_config.h"
#include "audio_fade.h"
#include "audio_position.h"

namespace Halley
{
	class AudioEmitter;
	class AudioObject;
	class AudioPosition;
	class AudioEngine;
	class ConfigNode;
	class ConfigFile;
	class ResourceLoader;
	class IAudioEventAction;
	class Resources;
	class AudioDynamicsConfig;

	enum class AudioEventActionType
	{
		Play,
		Stop,
		Pause,
		Resume,
		SetVolume,
		SetSwitch,
		SetVariable
	};

	class AudioEvent final : public Resource
	{
	public:
		AudioEvent();
		AudioEvent(const AudioEvent& other);
		AudioEvent(AudioEvent&& other) = default;
		explicit AudioEvent(const ConfigNode& config);

		AudioEvent& operator=(const AudioEvent& other);
		AudioEvent& operator=(AudioEvent&& other) = default;

		size_t run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const;

		const Vector<std::unique_ptr<IAudioEventAction>>& getActions() const;
		Vector<std::unique_ptr<IAudioEventAction>>& getActions();

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		void reload(Resource&& resource) override;
		static std::shared_ptr<AudioEvent> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::AudioEvent; }

		void makeDefault();
        String toYAML() const;

	private:
		Vector<std::unique_ptr<IAudioEventAction>> actions;
		
		void loadDependencies(Resources& resources);
		std::unique_ptr<IAudioEventAction> makeAction(AudioEventActionType type) const;
	};
	
	template <>
	struct EnumNames<AudioEventActionType> {
		constexpr std::array<const char*, 7> operator()() const {
			return{{
				"play",
				"stop",
				"pause",
				"resume",
				"setVolume",
				"setSwitch",
				"setVariable"
			}};
		}
	};

	class IAudioEventAction
	{
	public:
		virtual ~IAudioEventAction() {}
		virtual void load(const ConfigNode& config) = 0;
		virtual bool run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const = 0;
		virtual AudioEventActionType getType() const = 0;

		virtual void serialize(Serializer& s) const = 0;
		virtual void deserialize(Deserializer& s) = 0;
		virtual void loadDependencies(Resources& resources) {}

		virtual ConfigNode toConfigNode() const;
	};

	class AudioEventActionObject : public IAudioEventAction
	{
	public:
		void loadObject(const ConfigNode& config, bool loadObject = true);
		
		void serialize(Serializer& s) const override;
		void deserialize(Deserializer& s) override;

		void loadDependencies(Resources& resources) override;

		ConfigNode toConfigNode() const override;

		const String& getObjectName() const;
		void setObjectName(const String& name, Resources& resources);

		const AudioFade& getFade() const;
		AudioFade& getFade();

	protected:
		std::shared_ptr<const AudioObject> object;
		String objectName;
		AudioFade fade;
	};

	class AudioEventActionPlay final : public AudioEventActionObject
	{
	public:
		void load(const ConfigNode& config) override;

		bool run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const override;
		AudioEventActionType getType() const override { return AudioEventActionType::Play; }

		float getDelay() const;
		void setDelay(float delay);
		Range<float> getGain() const;
		Range<float>& getGain();
		void setGain(Range<float> gain);

		void serialize(Serializer& s) const override;
		void deserialize(Deserializer& s) override;

		void loadDependencies(Resources& resources) override;

		ConfigNode toConfigNode() const override;

	private:
		bool legacy = false;
		Range<float> playGain;
		float delay = 0;
	};

	class AudioEventActionStop final : public AudioEventActionObject
	{
	public:
		void load(const ConfigNode& config) override;

		bool run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const override;
		AudioEventActionType getType() const override { return AudioEventActionType::Stop; }
	};

	class AudioEventActionPause final : public AudioEventActionObject
	{
	public:
		void load(const ConfigNode& config) override;

		bool run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const override;
		AudioEventActionType getType() const override { return AudioEventActionType::Pause; }
	};

	class AudioEventActionResume final : public AudioEventActionObject
	{
	public:
		void load(const ConfigNode& config) override;

		bool run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const override;
		AudioEventActionType getType() const override { return AudioEventActionType::Resume; }
	};

	class AudioEventActionSetVolume final : public AudioEventActionObject
	{
	public:
		void load(const ConfigNode& config) override;

		bool run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const override;
		AudioEventActionType getType() const override { return AudioEventActionType::SetVolume; }

		void serialize(Serializer& s) const override;
		void deserialize(Deserializer& s) override;

		ConfigNode toConfigNode() const override;

	private:
		float gain;
	};

	class AudioEventActionSetSwitch final : public IAudioEventAction
	{
	public:
		void load(const ConfigNode& config) override;

		bool run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const override;
		AudioEventActionType getType() const override { return AudioEventActionType::SetSwitch; }

		void serialize(Serializer& s) const override;
		void deserialize(Deserializer& s) override;

		ConfigNode toConfigNode() const override;

	private:
		String switchId;
		String value;
	};

	class AudioEventActionSetVariable final : public IAudioEventAction
	{
	public:
		void load(const ConfigNode& config) override;

		bool run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const override;
		AudioEventActionType getType() const override { return AudioEventActionType::SetVariable; }

		void serialize(Serializer& s) const override;
		void deserialize(Deserializer& s) override;

		ConfigNode toConfigNode() const override;

	private:
		String variableId;
		float value;
	};
}
