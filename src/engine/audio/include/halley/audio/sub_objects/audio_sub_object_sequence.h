#pragma once
#include "../audio_fade.h"
#include "../audio_sub_object.h"

namespace Halley {
    enum class AudioSequenceType {
	    Sequential,
        Shuffle,
		ShuffleOnce,
		Random
    };
	
	template <>
	struct EnumNames<AudioSequenceType> {
		constexpr std::array<const char*, 4> operator()() const {
			return{{
				"sequential",
				"shuffle",
				"shuffleOnce",
				"random"
			}};
		}
	};

    class AudioSubObjectSequence final : public IAudioSubObject {
    public:
		struct Segment {
			AudioSubObjectHandle object;
			int endSample = 0;

			Segment() = default;
			Segment(const ConfigNode& node);
			Segment(AudioSubObjectHandle subObject);
			ConfigNode toConfigNode() const;

			void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);
		};

    	void load(const ConfigNode& node) override;
    	ConfigNode toConfigNode() const override;

        AudioSubObjectType getType() const override { return AudioSubObjectType::Sequence; }
	    std::unique_ptr<AudioSource> makeSource(AudioEngine& engine, AudioEmitter& emitter) const override;

        String getName() const override;
		const String& getRawName() const;
		void setName(String name);
		size_t getNumSubObjects() const override;
		const AudioSubObjectHandle& getSubObject(size_t n) const;
		AudioSubObjectHandle& getSubObject(size_t n) override;
		bool canAddObject(AudioSubObjectType type, const std::optional<String>& caseName) const override;
		void addObject(AudioSubObjectHandle handle, const std::optional<String>& caseName, size_t idx) override;
		AudioSubObjectHandle removeObject(const IAudioObject* object) override;

        void loadDependencies(Resources& resources) override;
	    void serialize(Serializer& s) const override;
	    void deserialize(Deserializer& s) override;

        AudioFade& getCrossFade();
		AudioSequenceType& getSequenceType();
		const AudioFade& getCrossFade() const;
		AudioSequenceType getSequenceType() const;

		gsl::span<const Segment> getSegments() const;
		gsl::span<Segment> getSegments();
		Segment& getSegment(size_t idx);

    private:
		String name;
        Vector<Segment> segments;

        AudioFade crossFade;
        AudioSequenceType sequenceType;
    };
}
