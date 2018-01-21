#include "ui_stylesheet.h"
#include "halley/file_formats/config_file.h"
#include "halley/audio/audio_clip.h"
#include "halley/core/graphics/text/font.h"
#include "halley/core/resources/resources.h"
#include "halley/support/logger.h"
using namespace Halley;

UIStyleSheet::UIStyleSheet()
{
}

UIStyleSheet::UIStyleSheet(const ConfigNode& node, Resources& resources)
{
	auto& UIStyleSheet = node["uiStyle"];

	if (UIStyleSheet.hasKey("sprites")) {
		for (auto& spriteNode: UIStyleSheet["sprites"].asMap()) {
			String name = spriteNode.first;
			if (spriteNode.second.getType() == ConfigNodeType::String) {
				if (spriteNode.second.asString().isEmpty()) {
					sprites[name] = defaultSprite;
				} else {
					sprites[name] = Sprite().setImage(resources, spriteNode.second.asString()).setSlicedFromMaterial();
				}
			} else {
				sprites[name] = Sprite().setImage(resources, spriteNode.second["img"].asString()).setColour(Colour4f::fromString(spriteNode.second["colour"].asString("#FFFFFF"))).setSlicedFromMaterial();
			}
		}
	}

	if (UIStyleSheet.hasKey("textRenderers")) {
		for (auto& textNode: UIStyleSheet["textRenderers"].asMap()) {
			String name = textNode.first;
			auto& text = textNode.second;
			textRenderers[name] = TextRenderer().setFont(resources.get<Font>(text["font"].asString())).setSize(text["size"].asFloat()).setColour(Colour4f::fromString(text["colour"].asString()));
		}
	}

	if (UIStyleSheet.hasKey("audioClips")) {
		for (auto& n: UIStyleSheet["audioClips"].asMap()) {
			String clip = n.second.asString();
			if (clip.isEmpty()) {
				audioClips[n.first] = {};
			} else {
				audioClips[n.first] = resources.get<AudioClip>(clip);
			}
		}
	}

	if (UIStyleSheet.hasKey("borders")) {
		for (auto& n: UIStyleSheet["borders"].asMap()) {
			auto vals = n.second.asSequence();
			borders[n.first] = Vector4f(vals[0].asFloat(), vals[1].asFloat(), vals[2].asFloat(), vals[3].asFloat());
		}
	}

	if (UIStyleSheet.hasKey("floats")) {
		for (auto& n: UIStyleSheet["floats"].asMap()) {
			floats[n.first] = n.second.asFloat();
		}
	}
}

void UIStyleSheet::setParent(std::shared_ptr<UIStyleSheet> p)
{
	parent = p;
}

const Sprite& UIStyleSheet::getSprite(const String& name)
{
	auto iter = sprites.find(name);
	if (iter != sprites.end()) {
		return iter->second;
	}
	if (parent) {
		return parent->getSprite(name);
	} else {
		Logger::logWarning("Sprite not found in UI style: " + name);
		return defaultSprite;
	}
}

const TextRenderer& UIStyleSheet::getTextRenderer(const String& name)
{
	auto iter = textRenderers.find(name);
	if (iter != textRenderers.end()) {
		return iter->second;
	}
	if (parent) {
		return parent->getTextRenderer(name);
	} else {
		Logger::logWarning("Text renderer not found in UI style: " + name);
		return defaultText;
	}
}

Vector4f UIStyleSheet::getBorder(const String& name)
{
	auto iter = borders.find(name);
	if (iter != borders.end()) {
		return iter->second;
	}
	if (parent) {
		return parent->getBorder(name);
	} else {
		Logger::logWarning("Border not found in UI style: " + name);
		return {};
	}
}

std::shared_ptr<const AudioClip> UIStyleSheet::getAudioClip(const String& name)
{
	auto iter = audioClips.find(name);
	if (iter != audioClips.end()) {
		return iter->second;
	}
	if (parent) {
		return parent->getAudioClip(name);
	} else {
		Logger::logWarning("Audio clip not found in UI style: " + name);
		return {};
	}
}

float UIStyleSheet::getFloat(const String& name)
{
	auto iter = floats.find(name);
	if (iter != floats.end()) {
		return iter->second;
	}
	if (parent) {
		return parent->getFloat(name);
	} else {
		Logger::logWarning("Float not found in UI style: " + name);
		return 0.0f;
	}
}
