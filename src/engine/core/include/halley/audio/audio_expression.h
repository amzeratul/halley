#pragma once
#include "halley/data_structures/vector.h"
#include "halley/maths/interpolation_curve.h"
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

    enum class AudioExpressionTermComp: uint8_t {
	    Equals,
        NotEquals
    };

	template <>
	struct EnumNames<AudioExpressionTermComp> {
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
        AudioExpressionTermComp op = AudioExpressionTermComp::Equals;
        String id;
        String value;
        InterpolationCurve points;
        float gain = 1;

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

    enum class AudioExpressionOperation: uint8_t {
	    Multiply,
        Add,
        Min,
        Max
    };

	template <>
	struct EnumNames<AudioExpressionOperation> {
		constexpr std::array<const char*, 4> operator()() const {
			return{{
				"multiply",
                "add",
                "min",
                "max"
			}};
		}
	};

	class AudioExpression {
    public:
        void load(const ConfigNode& node);
        ConfigNode toConfigNode() const;

        float evaluate(const AudioEmitter& emitter) const;

        void serialize(Serializer& s) const;
        void deserialize(Deserializer& s);

        Vector<AudioExpressionTerm>& getTerms();
        AudioExpressionOperation getOperation() const;
        void setOperation(AudioExpressionOperation op);

	private:
        Vector<AudioExpressionTerm> terms;
        AudioExpressionOperation operation = AudioExpressionOperation::Multiply;
    };
}
