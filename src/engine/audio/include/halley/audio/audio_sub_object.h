#pragma once
#include "audio_source.h"
#include "halley/data_structures/config_node.h"

namespace Halley {
	class AudioProperties;
	class IAudioSubObject;
	class AudioSubObjectHandle;
	class AudioEmitter;

	enum class AudioSubObjectType {
		None,
		Clips,
		Layers,
		Switch,
		Sequence
	};

	template <>
	struct EnumNames<AudioSubObjectType> {
		constexpr std::array<const char*, 5> operator()() const {
			return{{
				"none",
				"clips",
				"layers",
				"switch",
				"sequence"
			}};
		}
	};

	class IAudioObject {
	public:
		virtual ~IAudioObject() = default;

		virtual AudioSubObjectType getType() const = 0;
		virtual size_t getNumSubObjects() const;
		virtual AudioSubObjectHandle& getSubObject(size_t n);
		virtual Vector<String> getSubCategories(const AudioProperties& audioProperties) const;
		virtual String getSubObjectCategory(size_t n) const;
		virtual gsl::span<const String> getClips() const;
		virtual bool canCollapseToClip() const;
		virtual bool canAddObject(AudioSubObjectType type, const std::optional<String>& caseName) const;
		virtual void addObject(AudioSubObjectHandle handle, const std::optional<String>& caseName, size_t idx);
		virtual void addClip(std::shared_ptr<const AudioClip> clip, const std::optional<String>& caseName, size_t idx);
		virtual AudioSubObjectHandle removeObject(const IAudioObject* object);
		virtual void removeClip(const String& clipId);
		virtual void swapClips(size_t idxA, size_t idxB);
	};

	class IAudioSubObject : public IAudioObject {
	public:
		static std::unique_ptr<IAudioSubObject> makeSubObject(AudioSubObjectType type);
		static void copySubObject(IAudioSubObject& dst, const IAudioSubObject& src);
		static std::unique_ptr<IAudioSubObject> makeSubObject(const ConfigNode& node);

		virtual ~IAudioSubObject() = default;

		virtual void load(const ConfigNode& node) = 0;

		virtual std::unique_ptr<AudioSource> makeSource(AudioEngine& engine, AudioEmitter& emitter) const = 0;
		virtual void loadDependencies(Resources& resources) = 0;

		virtual void serialize(Serializer& s) const = 0;
		virtual void deserialize(Deserializer& s) = 0;

		virtual ConfigNode toConfigNode() const = 0;

		virtual String getName() const = 0;
	};

	class AudioSubObjectHandle {
	public:
		AudioSubObjectHandle() = default;
		AudioSubObjectHandle(std::unique_ptr<IAudioSubObject> obj);
		AudioSubObjectHandle(const ConfigNode& node);

		AudioSubObjectHandle(const AudioSubObjectHandle& other);
		AudioSubObjectHandle(AudioSubObjectHandle&& other) = default;
		AudioSubObjectHandle& operator=(const AudioSubObjectHandle& other);
		AudioSubObjectHandle& operator=(AudioSubObjectHandle&& other) = default;
		
		ConfigNode toConfigNode() const;

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

}
