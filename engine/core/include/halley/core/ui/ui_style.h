#pragma once

#include "halley/core/graphics/sprite/sprite.h"
#include "halley/core/graphics/text/text_renderer.h"
#include "halley/data_structures/flat_map.h"

namespace Halley {
	class ConfigNode;
	class AudioClip;

	class UIStyle {
	public:
		UIStyle();
		UIStyle(const ConfigNode& node, Resources& resources);

		void setParent(UIStyle& parent);

		const Sprite& getSprite(const String& name);
		const TextRenderer& getTextRenderer(const String& name);
		Vector4f getBorder(const String& name);
		std::shared_ptr<const AudioClip> getAudioClip(const String& name);

	private:
		UIStyle* parent = nullptr;
		FlatMap<String, Sprite> sprites;
		FlatMap<String, TextRenderer> textRenderers;
		FlatMap<String, Vector4f> borders;
		FlatMap<String, std::shared_ptr<const AudioClip>> audioClips;

		TextRenderer defaultText;
		Sprite defaultSprite;
	};
}
