#pragma once
#include "audio_source.h"
#include "halley/data_structures/config_node.h"

namespace Halley {
	class AudioEmitter;

	enum class AudioSubObjectType {
		None,
		Clips
	};

	template <>
	struct EnumNames<AudioSubObjectType> {
		constexpr std::array<const char*, 2> operator()() const {
			return{{
				"none",
				"clips"
			}};
		}
	};

	class IAudioSubObject {
	public:
		static std::unique_ptr<IAudioSubObject> makeSubObject(AudioSubObjectType type);
		static std::unique_ptr<IAudioSubObject> makeSubObject(const ConfigNode& node);

		virtual ~IAudioSubObject() = default;

		virtual void load(const ConfigNode& node) = 0;
		virtual AudioSubObjectType getType() = 0;

		virtual std::unique_ptr<AudioSource> makeSource(AudioEngine& engine, AudioEmitter& emitter) const = 0;
		virtual void loadDependencies(Resources& resources) = 0;

		virtual void serialize(Serializer& s) const = 0;
		virtual void deserialize(Deserializer& s) = 0;
	};

	class AudioSubObjectHandle {
	public:
		AudioSubObjectHandle() = default;
		AudioSubObjectHandle(std::unique_ptr<IAudioSubObject> obj);

		IAudioSubObject& getObject();
		const IAudioSubObject& getObject() const;

		IAudioSubObject& operator*();
		IAudioSubObject& operator*() const;

		IAudioSubObject* operator->();
		IAudioSubObject* operator->() const;
		
		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		bool hasValue() const;


	private:
		std::unique_ptr<IAudioSubObject> obj;
	};

	class AudioSubObjectClips final : public IAudioSubObject {
	public:
		void load(const ConfigNode& node) override;

		AudioSubObjectType getType() override { return AudioSubObjectType::Clips; }

		std::unique_ptr<AudioSource> makeSource(AudioEngine& engine, AudioEmitter& emitter) const override;
		void loadDependencies(Resources& resources) override;

		void serialize(Serializer& s) const override;
		void deserialize(Deserializer& s) override;

	private:
		Vector<String> clips;
		Vector<std::shared_ptr<const AudioClip>> clipData;
		float delay = 0.0f;
		bool loop = false;
	};

}
