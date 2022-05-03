#pragma once
#include "halley/data_structures/vector.h"
#include "halley/maths/vector2.h"
#include "halley/text/string_converter.h"

namespace Halley {
	class ConfigNode;
	class AudioEmitter;
    class Serializer;
    class Deserializer;

    enum class AudioExpressionTermType: uint8_t {
	    Switch,
        Variable
    };

	template <>
	struct EnumNames<AudioExpressionTermType> {
		constexpr std::array<const char*, 2> operator()() const {
			return{{
				"switch",
                "variable"
			}};
		}
	};

    enum class AudioExpressionTermOp: uint8_t {
	    Equals,
        NotEquals
    };

	template <>
	struct EnumNames<AudioExpressionTermOp> {
		constexpr std::array<const char*, 2> operator()() const {
			return{{
				"equals",
                "notEquals"
			}};
		}
	};

    class AudioExpressionTerm {
    public:
        AudioExpressionTermType type = AudioExpressionTermType::Switch;
        AudioExpressionTermOp op = AudioExpressionTermOp::Equals;
        String id;
        String value;
        Vector<Vector2f> points;

        AudioExpressionTerm() = default;
        AudioExpressionTerm(AudioExpressionTermType type);
        AudioExpressionTerm(const ConfigNode& node);
        ConfigNode toConfigNode() const;

        float evaluate(const AudioEmitter& emitter) const;
        float evaluateSwitch(const AudioEmitter& emitter) const;
        float evaluateVariable(const AudioEmitter& emitter) const;

        void serialize(Serializer& s) const;
        void deserialize(Deserializer& s);
    };

	class AudioExpression {
    public:
        void load(const ConfigNode& node);
        ConfigNode toConfigNode() const;

        float evaluate(const AudioEmitter& emitter) const;

        void serialize(Serializer& s) const;
        void deserialize(Deserializer& s);

        Vector<AudioExpressionTerm>& getTerms();

	private:
        Vector<AudioExpressionTerm> terms;
    };
}
