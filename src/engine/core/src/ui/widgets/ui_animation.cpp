#include "halley/ui/widgets/ui_animation.h"
using namespace Halley;

UIAnimation::UIAnimation(String id, Vector2f size, Vector2f animationOffset, AnimationPlayer animation)
	: UIAnimation(std::move(id), size, std::nullopt, animationOffset, std::move(animation))
{
}

UIAnimation::UIAnimation(String id, Vector2f size, std::optional<UISizer> sizer, Vector2f animationOffset, AnimationPlayer animation)
	: UIWidget(std::move(id), size, std::move(sizer))
	, offset(animationOffset)
	, scale(Vector2f(1, 1))
	, animation(std::move(animation))
	, colour(Colour4f(1, 1, 1, 1))
{
}

AnimationPlayer& UIAnimation::getPlayer()
{
	return animation;
}

const AnimationPlayer& UIAnimation::getPlayer() const
{
	return animation;
}

Sprite& UIAnimation::getSprite()
{
	return sprite;
}

const Sprite& UIAnimation::getSprite() const
{
	return sprite;
}

Vector2f UIAnimation::getOffset() const
{
	return offset;
}

void UIAnimation::setOffset(Vector2f o)
{
	offset = o;
}

void UIAnimation::setColour(Colour4f colour)
{
	this->colour = colour;
}

Colour4f UIAnimation::getColour() const
{
	return colour;
}

bool UIAnimation::isMouseInside(Vector2f mousePos) const
{
	return UIWidget::isMouseInside(mousePos) && sprite.hasPointVisible(mousePos);
}

void UIAnimation::setScale(Vector2f scale)
{
	this->scale = scale;
}

Vector2f UIAnimation::getScale() const
{
	return scale;
}

void UIAnimation::setAnimateWhileInvisible(bool animate)
{
	animateWhileInvisible = animate;
}

void UIAnimation::setDynamicValue(std::string_view key, ConfigNode value)
{
	if (key == "alpha") {
		colour.a = value.asFloat(1.0f);
	}
}

void UIAnimation::update(Time t, bool moved)
{
	if (animation.hasAnimation()) {
		if (colour.a >= 0.001f || animateWhileInvisible) {
			animation.update(t);
		}
		animation.updateSprite(sprite);
		sprite.setPos(getPosition() + offset).setColour(colour).setScale(scale);
	}
}

void UIAnimation::draw(UIPainter& painter) const
{
	if (animation.hasAnimation()) {
		painter.draw(sprite);
	}
}
