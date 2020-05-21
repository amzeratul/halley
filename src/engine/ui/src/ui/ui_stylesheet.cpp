#include "ui_stylesheet.h"
#include "halley/file_formats/config_file.h"
#include "halley/audio/audio_clip.h"
#include "halley/core/graphics/text/font.h"
#include "halley/core/resources/resources.h"
#include "halley/support/logger.h"
#include "halley/maths/vector4.h"
using namespace Halley;

template <typename T>
void loadStyleData(Resources& resources, const String& name, const ConfigNode& node, T& data) {}

template <>
void loadStyleData(Resources& resources, const String& name, const ConfigNode& node, Sprite& data)
{
	if (node.getType() == ConfigNodeType::String) {
		if (!node.asString().isEmpty()) {
			data = Sprite().setImage(resources, node.asString());
		}
	} else {
		data = Sprite()
			.setImage(resources, node["img"].asString())
			.setColour(Colour4f::fromString(node["colour"].asString("#FFFFFF")));
	}
}

template <>
void loadStyleData(Resources& resources, const String& name, const ConfigNode& node, TextRenderer& data)
{
	data = TextRenderer()
		.setFont(resources.get<Font>(node["font"].asString()))
		.setSize(node["size"].asFloat())
		.setColour(Colour4f::fromString(node["colour"].asString()))
		.setOutline(node["outline"].asFloat(0.0f))
		.setOutlineColour(Colour4f::fromString(node["outlineColour"].asString("#000000")))		
		.setAlignment(node["alignment"].asFloat(0.0f))
		.setSmoothness(node["smoothness"].asFloat(1.0));
}

template <>
void loadStyleData(Resources& resources, const String& name, const ConfigNode& node, String& data)
{
	if (node.asString().isEmpty()) {
		data = "";
	} else {
		data = node.asString();
	}
}

template <>
void loadStyleData(Resources& resources, const String& name, const ConfigNode& node, Vector4f& data)
{
	auto& vals = node.asSequence();
	data = Vector4f(vals[0].asFloat(), vals[1].asFloat(), vals[2].asFloat(), vals[3].asFloat());
}

template <>
void loadStyleData(Resources& resources, const String& name, const ConfigNode& node, float& data)
{
	data = node.asFloat();
}

template <>
void loadStyleData(Resources& resources, const String& name, const ConfigNode& node, Colour4f& data)
{
	data = Colour4f::fromString(node.asString());
}

template <>
void loadStyleData(Resources& resources, const String& name, const ConfigNode& node, std::shared_ptr<const UIStyleDefinition>& data)
{
	if (node.getType() != ConfigNodeType::Map) {
		data = {};
	} else {
		data = std::make_shared<UIStyleDefinition>(name, node, resources);
	}
}

template <typename T>
const T& getValue(const ConfigNode& node, Resources& resources, const String& name, const String& key, std::unordered_map<String, T>& cache)
{
	// Is it already in cache?
	const auto iter = cache.find(key);
	if (iter != cache.end()) {
		return iter->second;
	}

	// Not in cache, try to load it
	if (node.hasKey(key)) {
		T data;
		loadStyleData(resources, key, node[key], data);
		cache[key] = data;
		return cache[key];
	} else {
		// Not found. Use a default.
		const auto iter2 = cache.find(":default");
		if (iter2 != cache.end()) {
			Logger::logWarning(String(typeid(T).name()) + " not found in UI style: " + name + "." + key);
			return iter2->second;
		} else {
			throw Exception(String(typeid(T).name()) + " not found in UI style: " + name + "." + key + ". Additionally, default was not set.", HalleyExceptions::Tools);
		}
	}
}

template <typename T>
bool hasValue(const ConfigNode& node, Resources& resources, const String& name, const String& key, std::unordered_map<String, T>& cache)
{
	// Is it already in cache?
	const auto iter = cache.find(key);
	if (iter != cache.end()) {
		return true;
	}

	return node.hasKey(key);
}

class UIStyleDefinition::Pimpl {
public:
	mutable std::unordered_map<String, Sprite> sprites;
	mutable std::unordered_map<String, TextRenderer> textRenderers;
	mutable std::unordered_map<String, Vector4f> borders;
	mutable std::unordered_map<String, String> strings;
	mutable std::unordered_map<String, float> floats;
	mutable std::unordered_map<String, Colour4f> colours;
	mutable std::unordered_map<String, std::shared_ptr<const UIStyleDefinition>> subStyles;
};

UIStyleDefinition::UIStyleDefinition(String styleName, const ConfigNode& node, Resources& resources)
	: styleName(std::move(styleName))
	, node(node)
	, resources(resources)
	, pimpl(std::make_unique<Pimpl>())
{
	// Load defaults
	pimpl->sprites[":default"] = Sprite();
	pimpl->textRenderers[":default"] = TextRenderer();
	pimpl->floats[":default"] = 0.0f;
	pimpl->borders[":default"] = Vector4f();
	pimpl->strings[":default"] = "";
	pimpl->subStyles[":default"] = {};
	pimpl->colours[":default"] = Colour4f(1, 1, 1, 1);
}

UIStyleDefinition::~UIStyleDefinition() = default;

std::shared_ptr<const UIStyleDefinition> UIStyleDefinition::getSubStyle(const String& name) const
{
	return getValue(node, resources, styleName, name, pimpl->subStyles);
}

const Sprite& UIStyleDefinition::getSprite(const String& name) const
{
	return getValue(node, resources, styleName, name, pimpl->sprites);
}

const TextRenderer& UIStyleDefinition::getTextRenderer(const String& name) const
{
	return getValue(node, resources, styleName, name, pimpl->textRenderers);
}

bool UIStyleDefinition::hasTextRenderer(const String& name) const
{
	return hasValue(node, resources, styleName, name, pimpl->textRenderers);
}

bool UIStyleDefinition::hasColour(const String& name) const
{
	return hasValue(node, resources, styleName, name, pimpl->colours);
}

Vector4f UIStyleDefinition::getBorder(const String& name) const
{
	return getValue(node, resources, styleName, name, pimpl->borders);
}

const String& UIStyleDefinition::getString(const String& name) const
{
	return getValue(node, resources, styleName, name, pimpl->strings);
}

float UIStyleDefinition::getFloat(const String& name) const
{
	return getValue(node, resources, styleName, name, pimpl->floats);
}

Colour4f UIStyleDefinition::getColour(const String& name) const
{
	return getValue(node, resources, styleName, name, pimpl->colours);
}

UIStyleSheet::UIStyleSheet(Resources& resources)
	: resources(resources)
{
}

UIStyleSheet::UIStyleSheet(Resources& resources, const ConfigFile& file)
	: resources(resources)
{
	load(file);
}

void UIStyleSheet::load(const ConfigFile& file)
{
	load(file.getRoot());
	observers[file.getAssetId()] = ConfigObserver(file);
}

bool UIStyleSheet::needsUpdate() const
{
	for (auto& o: observers) {
		if (o.second.needsUpdate()) {
			return true;
		}
	}
	return false;
}

void UIStyleSheet::update()
{
	for (auto& o: observers) {
		o.second.update();
		load(o.second.getRoot());
	}
}

void UIStyleSheet::load(const ConfigNode& root)
{
	for (const auto& node: root["uiStyle"].asMap()) {
		styles[node.first] = std::make_unique<UIStyleDefinition>(node.first, node.second, resources);
	}
}

std::shared_ptr<const UIStyleDefinition> UIStyleSheet::getStyle(const String& styleName) const
{
	auto iter = styles.find(styleName);
	if (iter == styles.end()) {
		throw Exception("Unknown style: " + styleName, HalleyExceptions::UI);
	}
	return iter->second;
}
