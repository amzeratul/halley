#include "audio_expression.h"

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
		op = fromString<AudioExpressionTermOp>(node["op"].asString("equals"));
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
	if (op == AudioExpressionTermOp::Equals) {
		return isEqual ? 1.0f : 0.0f;
	} else if (op == AudioExpressionTermOp::NotEquals) {
		return isEqual ? 0.0f : 1.0f;
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
}

void AudioExpressionTerm::deserialize(Deserializer& s)
{
	int t;
	s >> t;
	type = static_cast<AudioExpressionTermType>(t);
	s >> t;
	op = static_cast<AudioExpressionTermOp>(t);
	s >> id;
	s >> value;
	s >> points;
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

Vector<AudioExpressionTerm>& AudioExpression::getTerms()
{
	return terms;
}
