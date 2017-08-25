#pragma once

#include "ui_stylesheet.h"

namespace Halley {
	class UIStyle {
	public:
		UIStyle();
		UIStyle(const String& baseName, std::shared_ptr<UIStyleSheet> styleSheet);

		UIStyle getSubStyle(const String& name) const;

		const Sprite& getSprite(const String& name);
		const TextRenderer& getTextRenderer(const String& name);
		Vector4f getBorder(const String& name);
		std::shared_ptr<const AudioClip> getAudioClip(const String& name);
		float getFloat(const String& name);

	private:
		String baseName;
		std::shared_ptr<UIStyleSheet> styleSheet;
	};
}
