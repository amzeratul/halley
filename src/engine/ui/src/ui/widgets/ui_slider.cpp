#include "halley/ui/widgets/ui_slider.h"
using namespace Halley;

UISlider::UISlider(const String& id, UIStyle style, float minValue, float maxValue, float value)
	: UIWidget(id, {}, UISizer(UISizerType::Horizontal))
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

void UISliderBar::draw(UIPainter& painter) const
{
	painter.draw(bar);
	painter.draw(barFull);
	painter.draw(thumb);
}

void UISliderBar::update(Time t, bool moved)
{
	const auto size = getSize();
	const float thumbX = size.x * parent.getRelativeValue();

	bar.scaleTo(size).setPos(getPosition());
	barFull.scaleTo(Vector2f(thumbX, size.y)).setPosition(getPosition());
	thumb.setPos(Vector2f(thumbX, 0) + getPosition());
}
