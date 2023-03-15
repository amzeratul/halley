#pragma once
#include "halley/resources/resource.h"
#include "halley/maths/range.h"
#include "audio_clip.h"
#include "audio_fade.h"

namespace Halley
{
	class AudioEmitter;
	class AudioObject;
	class AudioPosition;
	class AudioEngine;
	class ConfigNode;
	class ConfigFile;
	class ResourceLoader;
	class AudioEventAction;
	class Resources;

	enum class AudioEventActionType
	{
		PlayLegacy,
		Play,
		Stop,
		Pause,
		Resume,
		StopBus,
		PauseBus,
		ResumeBus,
		SetVolume,
		SetSwitch,
		SetVariable
	};
	
	template <>
	struct EnumNames<AudioEventActionType> {
		constexpr std::array<const char*, 11> operator()() const {
			return{{
				"playLegacy",
				"play",
				"stop",
				"pause",
				"resume",
				"stopBus",
				"pauseBus",
				"resumeBus",
				"setVolume",
				"setSwitch",
				"setVariable"
			}};
		}
	};

	enum class AudioEventScope {
		Global,
		Object
	};
	
	template <>
	struct EnumNames<AudioEventScope> {
		constexpr std::array<const char*, 2> operator()() const {
			return{{
				"global",
				"object"
			}};
		}
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

		size_t run(AudioEngine& engine, AudioEventId id, AudioEmitter& globalEmitter, AudioEmitter& objectEmitter) const;

		const Vector<std::unique_ptr<AudioEventAction>>& getActions() const;
		Vector<std::unique_ptr<AudioEventAction>>& getActions();

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		void reload(Resource&& resource) override;
		static std::shared_ptr<AudioEvent> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::AudioEvent; }

		void makeDefault();
        String toYAML() const;

		static std::unique_ptr<AudioEventAction> makeAction(AudioEventActionType type);
		static String getActionName(AudioEventActionType type);

	private:
		Vector<std::unique_ptr<AudioEventAction>> actions;
		
		void loadDependencies(Resources& resources);
	};

	class AudioEventAction
	{
	public:
		virtual ~AudioEventAction() {}
		virtual void load(const ConfigNode& config);
		virtual bool run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const = 0;
		virtual AudioEventActionType getType() const = 0;

		virtual AudioEventScope getScope() const { return scope; }
		void setScope(AudioEventScope scope) { this->scope = scope; }

		virtual void serialize(Serializer& s) const;
		virtual void deserialize(Deserializer& s);
		virtual void loadDependencies(Resources& resources) {}

		virtual ConfigNode toConfigNode() const;

	protected:
		AudioEventScope scope = AudioEventScope::Object;
	};

	class AudioEventActionObject : public AudioEventAction
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

	class AudioEventActionBus : public AudioEventAction {
	public:
		void load(const ConfigNode& config) override;
		ConfigNode toConfigNode() const override;

		void serialize(Serializer& s) const override;
		void deserialize(Deserializer& s) override;

		const String& getBusName() const;
		void setBusName(String name);

		AudioFade& getFade();

	protected:
		String busName;
		AudioFade fade;
	};

	class AudioEventActionPlay final : public AudioEventActionObject
	{
	public:
		AudioEventActionPlay(bool legacy);
		void load(const ConfigNode& config) override;

		bool run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const override;
		AudioEventActionType getType() const override;

		float getDelay() const;
		void setDelay(float delay);
		Range<float> getGain() const;
		Range<float>& getGain();
		void setGain(Range<float> gain);
		Range<float> getPitch() const;
		Range<float>& getPitch();
		void setPitch(Range<float> pitch);
		bool isSingleton() const;
		void setSingleton(bool value);

		void serialize(Serializer& s) const override;
		void deserialize(Deserializer& s) override;

		void loadDependencies(Resources& resources) override;

		ConfigNode toConfigNode() const override;

	private:
		bool legacy = false;
		bool singleton = false;
		Range<float> playGain;
		Range<float> playPitch;
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

	class AudioEventActionStopBus final : public AudioEventActionBus
	{
	public:
		void load(const ConfigNode& config) override;

		bool run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const override;
		AudioEventActionType getType() const override { return AudioEventActionType::StopBus; }
	};

	class AudioEventActionPauseBus final : public AudioEventActionBus
	{
	public:
		void load(const ConfigNode& config) override;

		bool run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const override;
		AudioEventActionType getType() const override { return AudioEventActionType::PauseBus; }
	};

	class AudioEventActionResumeBus final : public AudioEventActionBus
	{
	public:
		void load(const ConfigNode& config) override;

		bool run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const override;
		AudioEventActionType getType() const override { return AudioEventActionType::ResumeBus; }
	};

	class AudioEventActionSetVolume final : public AudioEventActionObject
	{
	public:
		void load(const ConfigNode& config) override;

		bool run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const override;
		AudioEventActionType getType() const override { return AudioEventActionType::SetVolume; }

		float getGain() const;
		void setGain(float value);

		void serialize(Serializer& s) const override;
		void deserialize(Deserializer& s) override;

		ConfigNode toConfigNode() const override;

	private:
		float gain = 1;
	};

	class AudioEventActionSetSwitch final : public AudioEventAction
	{
	public:
		void load(const ConfigNode& config) override;

		bool run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const override;
		AudioEventActionType getType() const override { return AudioEventActionType::SetSwitch; }

		const String& getSwitchId() const;
		const String& getValue() const;
		void setSwitchId(String id);
		void setValue(String val);

		void serialize(Serializer& s) const override;
		void deserialize(Deserializer& s) override;

		ConfigNode toConfigNode() const override;

	private:
		String switchId;
		String value;
	};

	class AudioEventActionSetVariable final : public AudioEventAction
	{
	public:
		void load(const ConfigNode& config) override;

		bool run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const override;
		AudioEventActionType getType() const override { return AudioEventActionType::SetVariable; }

		const String& getVariableId() const;
		float getValue() const;
		void setVariableId(String id);
		void setValue(float val);

		void serialize(Serializer& s) const override;
		void deserialize(Deserializer& s) override;

		ConfigNode toConfigNode() const override;

	private:
		String variableId;
		float value = 0;
	};
}
