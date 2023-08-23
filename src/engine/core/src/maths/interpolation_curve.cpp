#include "halley/maths/interpolation_curve.h"
#include "halley/data_structures/config_node.h"
#include "halley/bytes/byte_serializer.h"

using namespace Halley;

InterpolationCurve::InterpolationCurve()
{
	makeDefault();
}

InterpolationCurve::InterpolationCurve(const ConfigNode& node, bool startFromZero)
{
	if (node.getType() == ConfigNodeType::Map) {
		points = node["points"].asVector<Vector2f>({});
		tweens = node["tweens"].asVector<TweenCurve>({});
		scale = node["scale"].asFloat(1.0f);
	} else if (node.getType() == ConfigNodeType::Sequence) {
		points = node.asVector<Vector2f>();
		tweens.resize(points.size(), TweenCurve::Linear);
		scale = 1.0f;
	} else if (node.getType() == ConfigNodeType::Float || node.getType() == ConfigNodeType::Int || node.getType() == ConfigNodeType::String) {
		makeDefault(false);
		scale = node.asFloat(1.0f);
	} else {
		makeDefault(startFromZero);
	}
}

ConfigNode InterpolationCurve::toConfigNode() const
{
	ConfigNode::MapType result;
	result["points"] = points;
	result["tweens"] = tweens;
	result["scale"] = scale;
	return result;
}

void InterpolationCurve::makeDefault(bool startFromZero)
{
	scale = 1.0f;
	points.clear();
	tweens.clear();
	points.push_back(Vector2f(0, startFromZero ? 0.0f : 1.0f));
	points.push_back(Vector2f(1, 1));
	tweens.push_back(TweenCurve::Linear);
	tweens.push_back(TweenCurve::Linear);
}

bool InterpolationCurve::operator==(const InterpolationCurve& other) const
{
	return points == other.points && tweens == other.tweens && scale == other.scale;
}

bool InterpolationCurve::operator!=(const InterpolationCurve& other) const
{
	return !(*this == other);
}

void InterpolationCurve::serialize(Serializer& s) const
{
	s << points;
	s << tweens;
	s << scale;
}

void InterpolationCurve::deserialize(Deserializer& s)
{
	s >> points;
	s >> tweens;
	s >> scale;	
}

float InterpolationCurve::evaluate(float val) const
{
	return evaluateRaw(val) * scale;
}

float InterpolationCurve::evaluateRaw(float val) const
{
	if (points.empty()) {
		return 1.0f;
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
			return lerp(points[i - 1].y, points[i].y, Tween<float>::applyCurve(t, tweens[i]));
		}
	}

	// After last point
	return points.back().y;
}

PrecomputedInterpolationCurve::PrecomputedInterpolationCurve(const InterpolationCurve& src)
	: scale(src.scale)
{
	constexpr float mult = 1.0f / static_cast<float>(nElements);
	for (size_t i = 0; i < nElements; ++i) {
		const float t = static_cast<float>(i) * mult;
		elements[i] = static_cast<uint8_t>(clamp(src.evaluateRaw(t), 0.0f, 1.0f) * 255.5f);
	}
}

float PrecomputedInterpolationCurve::evaluate(float t) const
{
	const float val = t * static_cast<float>(nElements);
	const auto v0 = std::floor(val);
	const auto idx0 = clamp(static_cast<size_t>(v0), static_cast<size_t>(0), nElements);
	const auto idx1 = clamp(static_cast<size_t>(v0 + 1), static_cast<size_t>(0), nElements);

	constexpr bool interpolate = true;
	if (interpolate) {
		return lerp(static_cast<float>(elements[idx0]) / 255.0f, static_cast<float>(elements[idx1]) / 255.0f, val - v0);
	} else {
		return static_cast<float>(elements[idx0]) / 255.0f;
	}
}

ConfigNode ConfigNodeSerializer<InterpolationCurve>::serialize(const InterpolationCurve& curve, const EntitySerializationContext& context)
{
	return curve.toConfigNode();
}

InterpolationCurve ConfigNodeSerializer<InterpolationCurve>::deserialize(const EntitySerializationContext& context, const ConfigNode& node)
{
	return InterpolationCurve(node);
}
