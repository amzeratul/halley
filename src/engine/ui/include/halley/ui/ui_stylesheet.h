#pragma once

#include "halley/core/graphics/sprite/sprite.h"
#include "halley/core/graphics/text/text_renderer.h"
#include "halley/data_structures/flat_map.h"

namespace Halley {
	class ConfigNode;
	class AudioClip;
	class UISTyle;

	class UIStyleSheet {
		friend class UIStyle;

	public:
		UIStyleSheet();
		UIStyleSheet(const ConfigNode& node, Resources& resources);

		void setParent(std::shared_ptr<UIStyleSheet> parent);

	private:
		std::shared_ptr<UIStyleSheet> parent;
		FlatMap<String, Sprite> sprites;
		FlatMap<String, TextRenderer> textRenderers;
		FlatMap<String, Vector4f> borders;
		FlatMap<String, std::shared_ptr<const AudioClip>> audioClips;
		FlatMap<String, float> floats;

		TextRenderer defaultText;
		Sprite defaultSprite;

		const Sprite& getSprite(const String& name);
		const TextRenderer& getTextRenderer(const String& name);
		Vector4f getBorder(const String& name);
		std::shared_ptr<const AudioClip> getAudioClip(const String& name);
		float getFloat(const String& name);
	};
}
