#include "ui_style.h"
using namespace Halley;

UIStyle::UIStyle()
{
}

UIStyle::UIStyle(std::shared_ptr<const UIStyleDefinition> style)
	: style(std::move(style))
{
}

UIStyle::UIStyle(const String& styleName, std::shared_ptr<UIStyleSheet> styleSheet)
{
	Expects(styleSheet != nullptr);
	style = styleSheet->getStyle(styleName);
}

UIStyle UIStyle::getSubStyle(const String& name) const
{
	return UIStyle(style->getSubStyle(name));
}

const Sprite& UIStyle::getSprite(const String& name) const
{
	return style->getSprite(name);
}

const TextRenderer& UIStyle::getTextRenderer(const String& name) const
{
	return style->getTextRenderer(name);
}

bool UIStyle::hasTextRenderer(const String& name) const
{
	return style->hasTextRenderer(name);
}

Vector4f UIStyle::getBorder(const String& name) const
{
	return style->getBorder(name);
}

const String& UIStyle::getString(const String& name) const
{
	return style->getString(name);
}

float UIStyle::getFloat(const String& name) const
{
	return style->getFloat(name);
}

Colour4f UIStyle::getColour(const String& name) const
{
	return style->getColour(name);
}

bool UIStyle::hasColour(const String& name) const
{
	return style->hasColour(name);
}
