#include "halley/maths/colour_gradient.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/bytes/config_node_serializer.h"
#include "halley/maths/line.h"

using namespace Halley;

ColourGradient::ColourGradient()
{
	makeDefault();
}

ColourGradient::ColourGradient(const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Map) {
		positions = node["positions"].asVector<float>({});
		colours = node["colours"].asVector<Colour4f>({});
	} else {
		makeDefault();
	}
}

ColourGradient::ColourGradient(float fadeInEnd, float fadeOutStart)
{
	float fadeInValue = 1.0f;
	float fadeOutValue = 1.0f;
	if (fadeInEnd < 0) {
		fadeInEnd = 0;
	} else if (fadeInEnd > 1) {
		fadeInValue = 1.0f / fadeInEnd;
		fadeInEnd = 1.0f;
	}
	if (fadeOutStart > 1) {
		fadeOutStart = 1;
	} else if (fadeOutStart < 0) {
		fadeOutValue = 1.0f / (1 - fadeOutStart);
		fadeOutStart = 0;
	}

	if (fadeInEnd > fadeOutStart) {
		const auto a = Line(Vector2f(0, 0), Vector2f(fadeInEnd, fadeInValue).normalized());
		const auto b = Line(Vector2f(1, 0), Vector2f(fadeOutStart - 1.0f, fadeOutValue).normalized());
		if (const auto mid = a.intersection(b)) {
			add(Colour4f(1, 1, 1, mid->x), mid->y);
		}
	} else {
		add(Colour4f(1, 1, 1, fadeInValue), fadeInEnd);
		add(Colour4f(1, 1, 1, fadeOutValue), fadeOutStart);
	}
}

ConfigNode ColourGradient::toConfigNode() const
{
	ConfigNode::MapType result;
	result["positions"] = positions;
	result["colours"] = colours;
	return result;
}

void ColourGradient::makeDefault()
{
	positions.clear();
	positions.push_back(0);
	positions.push_back(1);
	colours.clear();
	colours.push_back(Colour4f(1, 1, 1, 1));
	colours.push_back(Colour4f(1, 1, 1, 1));
	dirty = true;
}

bool ColourGradient::operator==(const ColourGradient& other) const
{
	return colours == other.colours;
}

bool ColourGradient::operator!=(const ColourGradient& other) const
{
	return colours != other.colours;
}

void ColourGradient::serialize(Serializer& s) const
{
	s << colours;
}

void ColourGradient::deserialize(Deserializer& s)
{
	s >> colours;
}

Colour4f ColourGradient::evaluateSource(float val) const
{
	if (positions.empty()) {
		return Colour4f(0, 0, 0, 0);
	}

	// Before first point
	if (val < positions.front()) {
		if (positions.front() < 0.001f) {
			return colours.front();
		} 	else {
			return colours.front().multiplyAlpha(std::pow(val / positions.front(), 2.2f));
		}
	}

	// Between two positions
	for (size_t i = 1; i < positions.size(); ++i) {
		const float prevX = positions[i - 1];
		const float nextX = positions[i];

		if (val >= prevX && val <= nextX) {
			const float t = (val - prevX) / (nextX - prevX);
			assert(t >= 0.0f);
			assert(t <= 1.0f);
			constexpr bool useGamma = true;
			if (useGamma) {
				return lerp(colours[i - 1].applyGammaAlpha(1.0f / 2.2f), colours[i].applyGammaAlpha(1.0f / 2.2f), t).applyGammaAlpha(2.2f);
			} else {
				return lerp(colours[i - 1], colours[i], t);
			}
		}
	}

	// After last point
	if (positions.back() > 0.999f) {
		return colours.back();
	} else {
		return colours.back().multiplyAlpha(std::pow((1.0f - val) / (1.0f - positions.back()), 2.2f));
	}
}

Colour4f ColourGradient::evaluatePrecomputed(float val) const
{
	if (dirty) {
		precompute();
		dirty = false;
	}

	const auto maxIdx = precomputedSize - 1;
	const auto fractIdx = std::max(val, 0.0f) * static_cast<float>(maxIdx);
	const auto idx = clamp(static_cast<int>(fractIdx), 0, maxIdx);
	const auto next = std::min(idx + 1, maxIdx);

	return lerp(precomputed[idx], precomputed[next], fractIdx - idx);
}

void ColourGradient::markDirty()
{
	dirty = true;
}

void ColourGradient::precompute() const
{
	precomputed.resize(precomputedSize);
	const float scale = 1.0f / (precomputedSize - 1);
	for (int i = 0; i < precomputedSize; ++i) {
		precomputed[i] = evaluateSource(i * scale);
	}
}

void ColourGradient::render(Image& image)
{
	const auto size = image.getSize();
	auto dst = image.getPixels4BPP();
	image.setFormat(Image::Format::RGBA);

	for (int x = 0; x < size.x; ++x) {
		const float pos = static_cast<float>(x) / static_cast<float>(size.x - 1);
		const auto col = Image::convertColourToInt(evaluatePrecomputed(pos));
		for (int y = 0; y < size.y; ++y) {
			dst[x + y * size.x] = col;
		}
	}
	image.preMultiply();
}

void ColourGradient::clear()
{
	colours.clear();
	positions.clear();
	dirty = true;
}

void ColourGradient::add(Colour4f col, float position)
{
	colours.push_back(col);
	positions.push_back(position);
	dirty = true;
}

bool ColourGradient::isTrivial() const
{
	for (const auto& c: colours) {
		if (c != Colour4f(1, 1, 1, 1)) {
			return false;
		}
	}
	return true;
}
