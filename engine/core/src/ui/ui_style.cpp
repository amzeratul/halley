#include "ui/ui_style.h"
using namespace Halley;

UIStyle::UIStyle()
{
}

UIStyle::UIStyle(const String& baseName, std::shared_ptr<UIStyleSheet> styleSheet)
	: baseName(baseName + ".")
	, styleSheet(styleSheet)
{
	Expects(styleSheet);
}

UIStyle UIStyle::getSubStyle(const String& name) const
{
	return UIStyle(baseName + name, styleSheet);
}

const Sprite& UIStyle::getSprite(const String& name)
{
	return styleSheet->getSprite(baseName + name);
}

const TextRenderer& UIStyle::getTextRenderer(const String& name)
{
	return styleSheet->getTextRenderer(baseName + name);
}

Vector4f UIStyle::getBorder(const String& name)
{
	return styleSheet->getBorder(baseName + name);
}

std::shared_ptr<const AudioClip> UIStyle::getAudioClip(const String& name)
{
	return styleSheet->getAudioClip(baseName + name);
}

float UIStyle::getFloat(const String& name)
{
	return styleSheet->getFloat(baseName + name);
}
