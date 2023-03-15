#include "halley/audio/audio_expression.h"

#include "audio_emitter.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/data_structures/config_node.h"

using namespace Halley;

AudioExpressionTerm::AudioExpressionTerm(AudioExpressionTermType type)
	: type(type)
{
}

AudioExpressionTerm::AudioExpressionTerm(const ConfigNode& node)
{
	type = fromString<AudioExpressionTermType>(node["type"].asString());
	switch (type) {
	case AudioExpressionTermType::Switch:
		id = node["id"].asString();
		value = node["value"].asString();
		op = fromString<AudioExpressionTermComp>(node["op"].asString("equals"));
		gain = node["gain"].asFloat(1.0f);
		break;

	case AudioExpressionTermType::Variable:
		id = node["id"].asString();
		points = node["points"].asVector<Vector2f>();
		break;
	}
}

ConfigNode AudioExpressionTerm::toConfigNode() const
{
	ConfigNode::MapType result;
	result["type"] = toString(type);
	if (type == AudioExpressionTermType::Switch) {
		result["id"] = id;
		result["value"] = value;
		result["op"] = toString(op);
		result["gain"] = gain;
	} else if (type == AudioExpressionTermType::Variable) {
		result["id"] = id;
		result["points"] = points;
	}
	return result;
}

float AudioExpressionTerm::evaluate(const AudioEmitter& emitter) const
{
	switch (type) {
	case AudioExpressionTermType::Switch:
		return evaluateSwitch(emitter);
	case AudioExpressionTermType::Variable:
		return evaluateVariable(emitter);
	}

	return 1.0f;
}

float AudioExpressionTerm::evaluateSwitch(const AudioEmitter& emitter) const
{
	const bool isEqual = emitter.getSwitchValue(id) == value;
	if (op == AudioExpressionTermComp::Equals) {
		return isEqual ? gain : 0.0f;
	} else if (op == AudioExpressionTermComp::NotEquals) {
		return isEqual ? 0.0f : gain;
	}
	return 0.0f; // ??
}

float AudioExpressionTerm::evaluateVariable(const AudioEmitter& emitter) const
{
	const auto val = emitter.getVariableValue(id);

	// No points!
	if (points.empty()) {
		return val;
	}

	// Before first point
	if (val < points.front().x) {
		return points.front().y;
	}

	// Between two points
	for (size_t i = 1; i < points.size(); ++i) {
		const float prevX = points[i - 1].x;
		const float nextX = points[i].x;

		if (val >= prevX && val < nextX) {
			const float t = (val - prevX) / (nextX - prevX);
			assert(t >= 0.0f);
			assert(t <= 1.0f);
			return lerp(points[i - 1].y, points[i].y, t);
		}
	}

	// After last point
	return points.back().y;
}

void AudioExpressionTerm::serialize(Serializer& s) const
{
	s << static_cast<int>(type);
	s << static_cast<int>(op);
	s << id;
	s << value;
	s << points;
	s << gain;
}

void AudioExpressionTerm::deserialize(Deserializer& s)
{
	int t;
	s >> t;
	type = static_cast<AudioExpressionTermType>(t);
	s >> t;
	op = static_cast<AudioExpressionTermComp>(t);
	s >> id;
	s >> value;
	s >> points;
	s >> gain;
}

void AudioExpression::load(const ConfigNode& node)
{
	terms = node["terms"].asVector<AudioExpressionTerm>();
	operation = fromString<AudioExpressionOperation>(node["operation"].asString("multiply"));
}

ConfigNode AudioExpression::toConfigNode() const
{
	ConfigNode::MapType result;
	result["terms"] = terms;
	result["operation"] = toString(operation);
	return result;
}

float AudioExpression::evaluate(const AudioEmitter& emitter) const
{
	float value;

	switch (operation) {
	case AudioExpressionOperation::Multiply:
	case AudioExpressionOperation::Min:
		value = 1.0f;
		break;
	default:
		value = 0.0f;
		break;
	}

	for (auto& t: terms) {
		const auto v = t.evaluate(emitter);

		switch (operation) {
		case AudioExpressionOperation::Multiply:
			value *= v;
			break;
		case AudioExpressionOperation::Add:
			value += v;
			break;
		case AudioExpressionOperation::Max:
			value = std::max(value, v);
			break;
		case AudioExpressionOperation::Min:
			value = std::min(value, v);
			break;
		}

	}
	return clamp(value, 0.0f, 1.0f);
}

void AudioExpression::serialize(Serializer& s) const
{
	s << terms;
	s << operation;
}

void AudioExpression::deserialize(Deserializer& s)
{
	s >> terms;
	s >> operation;
}

Vector<AudioExpressionTerm>& AudioExpression::getTerms()
{
	return terms;
}

AudioExpressionOperation AudioExpression::getOperation() const
{
	return operation;
}

void AudioExpression::setOperation(AudioExpressionOperation op)
{
	operation = op;
}
