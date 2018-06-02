#pragma once

#include "ui_stylesheet.h"

namespace Halley {
	class UIStyle {
	public:
		UIStyle();
		UIStyle(std::shared_ptr<const UIStyleDefinition>);
		UIStyle(const String& name, std::shared_ptr<UIStyleSheet> styleSheet);

		UIStyle getSubStyle(const String& name) const;

		const Sprite& getSprite(const String& name) const;
		const TextRenderer& getTextRenderer(const String& name) const;
		Vector4f getBorder(const String& name) const;
		const String& getString(const String& name) const;
		float getFloat(const String& name) const;

	private:
		std::shared_ptr<const UIStyleDefinition> style;
	};
}
