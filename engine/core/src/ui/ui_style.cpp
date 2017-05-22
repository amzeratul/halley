#include "ui/ui_style.h"
#include "halley/file_formats/config_file.h"
#include "halley/audio/audio_clip.h"
#include "halley/core/graphics/text/font.h"
#include "resources/resources.h"
using namespace Halley;

UIStyle::UIStyle()
{
}

UIStyle::UIStyle(const ConfigNode& node, Resources& resources)
{
	auto& uiStyle = node["uiStyle"];

	if (uiStyle.hasKey("sprites")) {
		for (auto& spriteNode: uiStyle["sprites"].asMap()) {
			String name = spriteNode.first;
			if (spriteNode.second.getType() == ConfigNodeType::Scalar) {
				sprites[name] = Sprite().setImage(resources, spriteNode.second.asString()).setSlicedFromMaterial();
			} else {
				sprites[name] = Sprite().setImage(resources, spriteNode.second["img"].asString()).setColour(Colour4f::fromString(spriteNode.second["colour"].asString("#FFFFFF"))).setSlicedFromMaterial();
			}
		}
	}

	if (uiStyle.hasKey("textRenderers")) {
		for (auto& textNode: uiStyle["textRenderers"].asMap()) {
			String name = textNode.first;
			auto& text = textNode.second;
			textRenderers[name] = TextRenderer().setFont(resources.get<Font>(text["font"].asString())).setSize(text["size"].asFloat()).setColour(Colour4f::fromString(text["colour"].asString()));
		}
	}

	if (uiStyle.hasKey("audioClips")) {
		for (auto& n: uiStyle["audioClips"].asMap()) {
			String clip = n.second.asString();
			if (clip.isEmpty()) {
				audioClips[n.first] = {};
			} else {
				audioClips[n.first] = resources.get<AudioClip>(clip);
			}
		}
	}

	if (uiStyle.hasKey("borders")) {
		for (auto& n: uiStyle["borders"].asMap()) {
			auto vals = n.second.asSequence();
			borders[n.first] = Vector4f(vals[0].asFloat(), vals[1].asFloat(), vals[2].asFloat(), vals[3].asFloat());
		}
	}
}

void UIStyle::setParent(UIStyle& p)
{
	parent = &p;
}

const Sprite& UIStyle::getSprite(const String& name)
{
	auto iter = sprites.find(name);
	if (iter != sprites.end()) {
		return iter->second;
	}
	if (parent) {
		return parent->getSprite(name);
	} else {
		throw Exception("No sprite in UIStyle: " + name);
	}
}

const TextRenderer& UIStyle::getTextRenderer(const String& name)
{
	auto iter = textRenderers.find(name);
	if (iter != textRenderers.end()) {
		return iter->second;
	}
	if (parent) {
		return parent->getTextRenderer(name);
	} else {
		throw Exception("No text renderer in UIStyle: " + name);
	}
}

Vector4f UIStyle::getBorder(const String& name)
{
	auto iter = borders.find(name);
	if (iter != borders.end()) {
		return iter->second;
	}
	if (parent) {
		return parent->getBorder(name);
	} else {
		return {};
	}
}

std::shared_ptr<const AudioClip> UIStyle::getAudioClip(const String& name)
{
	auto iter = audioClips.find(name);
	if (iter != audioClips.end()) {
		return iter->second;
	}
	if (parent) {
		return parent->getAudioClip(name);
	} else {
		return {};
	}
}
