#pragma once
#include "halley/data_structures/vector.h"
#include "halley/maths/interpolation_curve.h"
#include "halley/maths/vector2.h"
#include "halley/text/string_converter.h"

namespace Halley {
	class AudioProperties;
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
		constexpr auto operator()() const {
			return std::to_array({
				"switch",
                "variable"
			});
		}
	};

    enum class AudioExpressionTermComp: uint8_t {
	    Equals,
        NotEquals
    };

	template <>
	struct EnumNames<AudioExpressionTermComp> {
		constexpr auto operator()() const {
			return std::to_array({
				"equals",
                "notEquals"
			});
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
        
        bool operator==(const AudioExpressionTerm& other) const;
        bool operator!=(const AudioExpressionTerm& other) const;

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
		constexpr auto operator()() const {
			return std::to_array({
				"multiply",
                "add",
                "min",
                "max"
			});
		}
	};

	class AudioExpression {
    public:
        void load(const ConfigNode& node);
        ConfigNode toConfigNode() const;

        float evaluate(const AudioEmitter& emitter, Range<float> range) const;
		void validate(const AudioProperties& audioProperties, const String& breadCrumbs) const;

        bool operator==(const AudioExpression& other) const;
        bool operator!=(const AudioExpression& other) const;

        void serialize(Serializer& s) const;
        void deserialize(Deserializer& s);

        Vector<AudioExpressionTerm>& getTerms();
        const Vector<AudioExpressionTerm>& getTerms() const;
        AudioExpressionOperation getOperation() const;
        void setOperation(AudioExpressionOperation op);

    private:
        Vector<AudioExpressionTerm> terms;
        AudioExpressionOperation operation = AudioExpressionOperation::Multiply;
    };
}
