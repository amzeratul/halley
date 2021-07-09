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

bool UIStyle::hasSubStyle(const String& name) const
{
	return style->hasSubStyle(name);
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

float UIStyle::getFloat(const String& name, float defaultValue) const
{
	return style->getFloat(name, defaultValue);
}

Vector2f UIStyle::getVector2f(const String& name) const
{
	return style->getVector2f(name);
}

Vector2f UIStyle::getVector2f(const String& name, Vector2f defaultValue) const
{	
	return style->getVector2f(name, defaultValue);
}

Colour4f UIStyle::getColour(const String& name) const
{
	return style->getColour(name);
}

bool UIStyle::hasColour(const String& name) const
{
	return style->hasColour(name);
}
