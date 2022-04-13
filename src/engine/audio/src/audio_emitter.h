#pragma once
#include "audio_position.h"
#include "audio_voice.h"

namespace Halley {
    class AudioEmitter {
    public:
        using VoiceCallback = std::function<void(AudioVoice&)>;

        AudioEmitter() = default;
        explicit AudioEmitter(AudioEmitterId id, AudioPosition position, bool temporary = false);

        [[nodiscard]] AudioEmitterId getId() const;

        [[nodiscard]] const AudioPosition& getPosition() const;
        void setPosition(const AudioPosition& pos);

        void addVoice(std::unique_ptr<AudioVoice> voice);
        void removeFinishedVoices();
        [[nodiscard]] gsl::span<const std::unique_ptr<AudioVoice>> getVoices() const;
    	void forVoices(AudioObjectId audioObjectId, VoiceCallback callback);

        bool shouldBeRemoved();
        void makeTemporary();

    private:
        AudioEmitterId id = 0;
        bool temporary = false;
        AudioPosition position;

        Vector<std::unique_ptr<AudioVoice>> voices;
    };
}
