#pragma once

#include "ui_stylesheet.h"

namespace Halley {
	class UIStyle {
	public:
		UIStyle();
		UIStyle(std::shared_ptr<UIStyleSheet> styleSheet);
		UIStyle(const String& baseName, std::shared_ptr<UIStyleSheet> styleSheet);

		UIStyle getSubStyle(const String& name) const;

		const Sprite& getSprite(const String& name) const;
		const TextRenderer& getTextRenderer(const String& name) const;
		Vector4f getBorder(const String& name) const;
		std::shared_ptr<const AudioClip> getAudioClip(const String& name) const;
		float getFloat(const String& name) const;

	private:
		String baseName;
		std::shared_ptr<UIStyleSheet> styleSheet;
	};
}
