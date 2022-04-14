#pragma once

namespace Halley {
	class ConfigNode;
	class AudioEmitter;
    class Serializer;
    class Deserializer;

	class AudioExpression {
    public:
        void load(const ConfigNode& node);

        float evaluate(const AudioEmitter& emitter) const;

        void serialize(Serializer& s) const;
        void deserialize(Deserializer& s);
    };
}
