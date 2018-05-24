#include "halley/ui/widgets/ui_slider.h"
using namespace Halley;

UISlider::UISlider(const String& id, UIStyle style, float minValue, float maxValue, float value)
	: UIWidget(id, {}, UISizer(UISizerType::Horizontal, 0), style.getBorder("innerBorder"))
	, minValue(minValue)
	, maxValue(maxValue)
{
	sliderBar = std::make_shared<UISliderBar>(*this, style);
	UIWidget::add(sliderBar, 1, {}, UISizerAlignFlags::CentreVertical | UISizerFillFlags::FillHorizontal);

	setValue(value);
}

void UISlider::setValue(float v)
{
	value = clamp(v, minValue, maxValue);

	notifyDataBind(value);
}

void UISlider::setRelativeValue(float v)
{
	setValue(lerp(minValue, maxValue, v));
}

float UISlider::getValue() const
{
	return value;
}

float UISlider::getMinValue() const
{
	return minValue;
}

float UISlider::getMaxValue() const
{
	return maxValue;
}

float UISlider::getRelativeValue() const
{
	return (value - minValue) / (maxValue - minValue);
}

void UISlider::readFromDataBind()
{
	auto data = getDataBind();
	if (data) {
		setValue(data->getFloatData());
	}
}

void UISlider::onManualControlAnalogueAdjustValue(float delta)
{
	setValue(getValue() + delta * 1.2f);
}

void UISlider::update(Time t, bool moved)
{
}

UISliderBar::UISliderBar(UISlider& parent, UIStyle style)
	: UIWidget("")
	, parent(parent)
{
	bar = style.getSprite("emptyBar");
	barFull = style.getSprite("fullBar");
	thumb = style.getSprite("thumb");
	left = style.getSprite("left");
	right = style.getSprite("right");
	extra = style.getBorder("extraMouseBorder");
	setMinSize(bar.getRawSize());
}

bool UISliderBar::canInteractWithMouse() const
{
	return true;
}

bool UISliderBar::isFocusLocked() const
{
	return held;
}

void UISliderBar::pressMouse(Vector2f mousePos, int button)
{
	if (button == 0 && isEnabled()) {
		held = true;
		auto relative = (mousePos - getPosition()) / getSize();
		parent.setRelativeValue(relative.x);
	}
}

void UISliderBar::releaseMouse(Vector2f mousePos, int button)
{
	if (button == 0 && isEnabled()) {
		held = false;
	}
}

void UISliderBar::onMouseOver(Vector2f mousePos)
{
	if (held) {
		auto relative = (mousePos - getPosition()) / getSize();
		parent.setRelativeValue(relative.x);
	}
}

Rect4f UISliderBar::getMouseRect() const
{
	return Rect4f(getPosition() - Vector2f(extra.x, extra.y), getPosition() + getSize() + Vector2f(extra.z, extra.w));
}

void UISliderBar::draw(UIPainter& painter) const
{
	painter.draw(left);
	painter.draw(right);
	fill(painter, Rect4f(getPosition(), getPosition() + getSize()), bar);
	fill(painter, Rect4f(getPosition(), getPosition() + getSize() * Vector2f(parent.getRelativeValue(), 1.0f)), barFull);
	painter.draw(thumb);
}

void UISliderBar::update(Time t, bool moved)
{
	const auto size = getSize();
	const float thumbX = size.x * parent.getRelativeValue();

	left.setPos(getPosition() + Vector2f(-left.getOriginalSize().x, 0));
	thumb.setPos(Vector2f(thumbX, 0) + getPosition());
	right.setPos(getPosition() + Vector2f(getSize().x, 0));
}

void UISliderBar::fill(UIPainter& painter, Rect4f rect, Sprite sprite) const
{
	auto p2 = painter.withClip(rect);
	const Vector2f spriteSize = sprite.getOriginalSize();
	int n = int(ceil(getSize().x / spriteSize.x));
	for (int i = 0; i < n; ++i) {
		sprite.setPos(getPosition() + Vector2f(i * spriteSize.x, 0));
		p2.draw(sprite, true);
	}
}
