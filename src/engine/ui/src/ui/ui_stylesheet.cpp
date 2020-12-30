#include "ui_stylesheet.h"

#include "ui_colour_scheme.h"
#include "halley/file_formats/config_file.h"
#include "halley/audio/audio_clip.h"
#include "halley/core/graphics/text/font.h"
#include "halley/core/resources/resources.h"
#include "halley/support/logger.h"
#include "halley/maths/vector4.h"
using namespace Halley;

Colour4f getColour(const ConfigNode& node, UIColourScheme* colourScheme)
{
	if (node.getType() != ConfigNodeType::String) {
		return Colour4f(1, 1, 1, 1);
	}
	
	const auto& str = node.asString();
	if (str.startsWith("#")) {
		return Colour4f::fromString(str);
	} else if (colourScheme) {
		return colourScheme->getColour(str);
	} else {
		Logger::logWarning("Invalid colour: \"" + str + "\"");
		return Colour4f();
	}
}

template <typename T>
void loadStyleData(Resources& resources, const String& name, const ConfigNode& node, UIColourScheme* colourScheme, T& data) {}

template <>
void loadStyleData(Resources& resources, const String& name, const ConfigNode& node, UIColourScheme* colourScheme, Sprite& data)
{
	if (node.getType() == ConfigNodeType::String) {
		if (!node.asString().isEmpty()) {
			data = Sprite().setImage(resources, node.asString());
		}
	} else {
		data = Sprite()
			.setImage(resources, node["img"].asString())
			.setColour(getColour(node["colour"], colourScheme));
	}
}

template <>
void loadStyleData(Resources& resources, const String& name, const ConfigNode& node, UIColourScheme* colourScheme, TextRenderer& data)
{
	data = TextRenderer()
		.setFont(resources.get<Font>(node["font"].asString()))
		.setSize(node["size"].asFloat())
		.setColour(getColour(node["colour"], colourScheme))
		.setOutline(node["outline"].asFloat(0.0f))
		.setOutlineColour(Colour4f::fromString(node["outlineColour"].asString("#000000")))		
		.setAlignment(node["alignment"].asFloat(0.0f))
		.setSmoothness(node["smoothness"].asFloat(1.0));
}

template <>
void loadStyleData(Resources& resources, const String& name, const ConfigNode& node, UIColourScheme* colourScheme, String& data)
{
	if (node.asString().isEmpty()) {
		data = "";
	} else {
		data = node.asString();
	}
}

template <>
void loadStyleData(Resources& resources, const String& name, const ConfigNode& node, UIColourScheme* colourScheme, Vector4f& data)
{
	auto& vals = node.asSequence();
	data = Vector4f(vals[0].asFloat(), vals[1].asFloat(), vals[2].asFloat(), vals[3].asFloat());
}

template <>
void loadStyleData(Resources& resources, const String& name, const ConfigNode& node, UIColourScheme* colourScheme, float& data)
{
	data = node.asFloat();
}

template <>
void loadStyleData(Resources& resources, const String& name, const ConfigNode& node, UIColourScheme* colourScheme, Colour4f& data)
{
	data = getColour(node, colourScheme);
}

template <>
void loadStyleData(Resources& resources, const String& name, const ConfigNode& node, UIColourScheme* colourScheme, std::shared_ptr<const UIStyleDefinition>& data)
{
	if (node.getType() != ConfigNodeType::Map) {
		data = {};
	} else {
		data = std::make_shared<UIStyleDefinition>(name, node, resources, colourScheme);
	}
}

template <typename T>
const T& getValue(const ConfigNode* node, Resources& resources, const String& name, const String& key, UIColourScheme* colourScheme, std::unordered_map<String, T>& cache)
{
	Expects(node);
	node->assertValid();
	
	// Is it already in cache?
	const auto iter = cache.find(key);
	if (iter != cache.end()) {
		return iter->second;
	}

	// Not in cache, try to load it
	if (node->hasKey(key)) {
		T data;
		loadStyleData(resources, key, (*node)[key], colourScheme, data);
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
bool hasValue(const ConfigNode* node, Resources& resources, const String& name, const String& key, std::unordered_map<String, T>& cache)
{
	// Is it already in cache?
	const auto iter = cache.find(key);
	if (iter != cache.end()) {
		return true;
	}

	return node->hasKey(key);
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

UIStyleDefinition::UIStyleDefinition(String styleName, const ConfigNode& node, Resources& resources, UIColourScheme* colourScheme)
	: styleName(std::move(styleName))
	, node(&node)
	, resources(resources)
	, colourScheme(colourScheme)
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
	return getValue(node, resources, styleName, name, colourScheme, pimpl->subStyles);
}

const Sprite& UIStyleDefinition::getSprite(const String& name) const
{
	return getValue(node, resources, styleName, name, colourScheme, pimpl->sprites);
}

const TextRenderer& UIStyleDefinition::getTextRenderer(const String& name) const
{
	return getValue(node, resources, styleName, name, colourScheme, pimpl->textRenderers);
}

bool UIStyleDefinition::hasTextRenderer(const String& name) const
{
	return hasValue(node, resources, styleName, name, pimpl->textRenderers);
}

bool UIStyleDefinition::hasColour(const String& name) const
{
	return hasValue(node, resources, styleName, name, pimpl->colours);
}

bool UIStyleDefinition::hasSubStyle(const String& name) const
{
	return hasValue(node, resources, styleName, name, pimpl->subStyles);
}

void UIStyleDefinition::reload(const ConfigNode& node)
{
	this->node = &node;
}

Vector4f UIStyleDefinition::getBorder(const String& name) const
{
	return getValue(node, resources, styleName, name, colourScheme, pimpl->borders);
}

const String& UIStyleDefinition::getString(const String& name) const
{
	return getValue(node, resources, styleName, name, colourScheme, pimpl->strings);
}

float UIStyleDefinition::getFloat(const String& name) const
{
	return getValue(node, resources, styleName, name, colourScheme, pimpl->floats);
}

Colour4f UIStyleDefinition::getColour(const String& name) const
{
	return getValue(node, resources, styleName, name, colourScheme, pimpl->colours);
}

UIStyleSheet::UIStyleSheet(Resources& resources)
	: resources(resources)
{
}

UIStyleSheet::UIStyleSheet(Resources& resources, const ConfigFile& file, UIColourScheme* colourScheme)
	: resources(resources)
{
	load(file, colourScheme);
}

void UIStyleSheet::load(const ConfigFile& file, UIColourScheme* colourScheme)
{
	load(file.getRoot(), colourScheme);
	observers[file.getAssetId()] = ConfigObserver(file);
}

bool UIStyleSheet::updateIfNeeded()
{
	if (needsUpdate()) {
		update();
		return true;
	}
	return false;
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
		load(o.second.getRoot(), lastColourScheme);
	}
}

void UIStyleSheet::load(const ConfigNode& root, UIColourScheme* colourScheme)
{
	lastColourScheme = colourScheme;
	
	for (const auto& node: root["uiStyle"].asMap()) {
		// If it already exists, update existing instance (as it might be kept by UI elements all around)
		const auto iter = styles.find(node.first);
		if (iter != styles.end()) {
			iter->second->reload(node.second);
		} else {
			styles[node.first] = std::make_unique<UIStyleDefinition>(node.first, node.second, resources, colourScheme);
		}
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
