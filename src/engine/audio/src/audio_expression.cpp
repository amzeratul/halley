#include "audio_expression.h"

#include "audio_emitter.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/data_structures/config_node.h"

using namespace Halley;

AudioExpressionTerm::AudioExpressionTerm(const ConfigNode& node)
{
	type = fromString<AudioExpressionTermType>(node["type"].asString());
	switch (type) {
	case AudioExpressionTermType::Switch:
		id = node["id"].asString();
		value = node["value"].asString();
		break;

	case AudioExpressionTermType::Variable:
		id = node["id"].asString();
		break;
	}
}

ConfigNode AudioExpressionTerm::toConfigNode() const
{
	ConfigNode::MapType result;
	result["type"] = toString(type);
	result["id"] = id;
	if (type == AudioExpressionTermType::Switch) {
		result["value"] = value;
	}
	return result;
}

float AudioExpressionTerm::evaluate(const AudioEmitter& emitter) const
{
	switch (type) {
	case AudioExpressionTermType::Switch:
		return emitter.getSwitchValue(id) == value ? 1.0f : 0.0f;
	case AudioExpressionTermType::Variable:
		return emitter.getVariableValue(id);
	}

	return 1.0f;
}

void AudioExpressionTerm::serialize(Serializer& s) const
{
	s << static_cast<int>(type);
	s << id;
	s << value;
}

void AudioExpressionTerm::deserialize(Deserializer& s)
{
	int t;
	s >> t;
	type = static_cast<AudioExpressionTermType>(t);
	s >> id;
	s >> value;
}

void AudioExpression::load(const ConfigNode& node)
{
	terms = node["terms"].asVector<AudioExpressionTerm>();
}

ConfigNode AudioExpression::toConfigNode() const
{
	ConfigNode::MapType result;
	result["terms"] = terms;
	return result;
}

float AudioExpression::evaluate(const AudioEmitter& emitter) const
{
	float value = 1.0f;
	for (auto& t: terms) {
		value *= t.evaluate(emitter);
	}
	return value;
}

void AudioExpression::serialize(Serializer& s) const
{
	s << terms;
}

void AudioExpression::deserialize(Deserializer& s)
{
	s >> terms;
}

gsl::span<AudioExpressionTerm> AudioExpression::getTerms()
{
	return terms;
}
