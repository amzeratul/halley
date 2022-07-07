#include "halley/ui/widgets/ui_slider.h"

#include "ui_validator.h"
#include "halley/ui/widgets/ui_label.h"
#include "halley/ui/widgets/ui_image.h"
#include "widgets/ui_spin_control2.h"
using namespace Halley;

UISlider::UISlider(const String& id, UIStyle style, float minValue, float maxValue, float value, bool hasSpinControl, bool allowFloat)
	: UIWidget(id, {}, UISizer(UISizerType::Horizontal, 0), style.getBorder("innerBorder"))
	, minValue(minValue)
	, maxValue(maxValue)
	, value(maxValue)
{
	styles.emplace_back(style);
	sliderBar = std::make_shared<UISliderBar>(*this, style);
	UIWidget::add(sliderBar, 1, style.getBorder("barBorder"), UISizerAlignFlags::CentreVertical | UISizerFillFlags::FillHorizontal);

	if (hasSpinControl) {
		spinControl = std::make_shared<UISpinControl2>(id + "_input", style.getSubStyle("spinControl"), value, allowFloat);
		spinControl->setMinimumValue(minValue);
		spinControl->setMaximumValue(maxValue);
		UIWidget::add(spinControl, 0, {}, UISizerAlignFlags::Centre);

		bindData(id + "_input", value, [=](float value)
		{
			fromInput = true;
			setValue(value);
			fromInput = false;
		});
	} else {
		box = std::make_shared<UIImage>(style.getSprite("labelBorder"), UISizer(UISizerType::Vertical), style.getBorder("labelInnerBorder"));
		label = std::make_shared<UILabel>(id + "_label", style, makeLabel());
		box->add(label, 0, {}, UISizerAlignFlags::Centre);
		box->layout();
		box->setMinSize(box->getSize());
		UIWidget::add(box, 0, {}, UISizerAlignFlags::Centre);
	}

	setHandle(UIEventType::MouseWheel, [=] (const UIEvent& event)
	{
		setValue(getValue() + float(event.getIntData()) * granularity.value_or(1.0f) * mouseWheelSpeed);
	});

	setValue(value);
}

void UISlider::setValue(float v)
{
	value = clamp(v, minValue, maxValue);
	if (box) {
		box->layout();
		box->setMinSize(Vector2f::max(box->getMinimumSize(), box->getSize()));
	}
	if (!fromInput) {
		updateLabel();
	}
	notifyDataBind(getValue());
}

void UISlider::setRelativeValue(float v)
{
	setValue(lerp(minValue, maxValue, v));
}

float UISlider::getValue(bool withGranularity) const
{
	float val = withGranularity ? getValueWithGranularity() : value;

	if (transformation) {
		val = transformation(val);
	}
	return val;
}

float UISlider::getMinValue() const
{
	return minValue;
}

float UISlider::getMaxValue() const
{
	return maxValue;
}

float UISlider::getRelativeValue(bool applyGranularity) const
{
	const float val = applyGranularity ? getValueWithGranularity() : value;
	return (val - minValue) / (maxValue - minValue);
}

void UISlider::readFromDataBind()
{
	auto data = getDataBind();
	if (data) {
		setValue(data->getFloatData());
	}
}

void UISlider::setGranularity(std::optional<float> g)
{
	granularity = g;
}

std::optional<float> UISlider::getGranularity() const
{
	return granularity;
}

void UISlider::setMouseWheelSpeed(float speed)
{
	mouseWheelSpeed = speed;
}

void UISlider::setShowLabel(bool show)
{
	if (box) {
		box->setActive(show);
	}
}

void UISlider::setLabelConversion(std::function<LocalisedString(float)> f)
{
	labelConversion = f;
	updateLabel();
}

void UISlider::setTransformation(std::function<float(float)> f)
{
	transformation = f;
}

void UISlider::onManualControlAnalogueAdjustValue(float input, Time t)
{
	timeSinceMove = 0;

	constexpr float targetSpeed = 3.0f;
	constexpr float accel = 3.0f;
	const float inputSpeed = input * targetSpeed;
	
	float speed = inputSpeed;
	if (speed * maxSpeed < 0) {
		maxSpeed = 0;
	}
	if (std::abs(speed) < std::abs(maxSpeed)) {
		maxSpeed = speed;
	}
	if (std::abs(speed) > std::abs(maxSpeed)) {
		speed = maxSpeed;
		maxSpeed = advance(maxSpeed, inputSpeed, float(t * accel));
	}

	setRelativeValue(getRelativeValue() + float(maxSpeed * t));
}

std::shared_ptr<UIWidget> UISlider::getLabelBox() const
{
	return box;
}

bool UISlider::canInteractWithMouse() const
{
	return true;
}

void UISlider::update(Time t, bool moved)
{
	timeSinceMove += t;
	if (timeSinceMove > 0.1f) {
		maxSpeed = 0;
	}
}

LocalisedString UISlider::makeLabel() const
{
	if (labelConversion) {
		return labelConversion(getValue());
	} else {
		return LocalisedString::fromNumber(int(lround(getValue())));
	}
}

void UISlider::updateLabel()
{
	if (box && label) {
		label->setText(makeLabel());
		box->layout();
		box->setMinSize(Vector2f::max(box->getMinimumSize(), box->getSize()));
	}
	if (spinControl) {
		spinControl->setValue(getValue());
	}
}

float UISlider::getValueWithGranularity() const
{
	if (granularity) {
		return clamp(lround((value - minValue) / granularity.value()) * granularity.value() + minValue, minValue, maxValue);
	} else {
		return value;
	}
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
	setMinSize(bar.getSize());
}

bool UISliderBar::canInteractWithMouse() const
{
	return true;
}

bool UISliderBar::isFocusLocked() const
{
	return held;
}

void UISliderBar::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
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

LocalisedString UISliderBar::getToolTip() const
{
	return parent.getToolTip();
}

void UISliderBar::draw(UIPainter& painter) const
{
	painter.draw(left);
	painter.draw(right);
	fill(painter, Rect4f(getPosition(), getPosition() + getSize()), bar);
	fill(painter, Rect4f(getPosition(), getPosition() + getSize() * Vector2f(parent.getRelativeValue(!held), 1.0f)), barFull);
	painter.draw(thumb);
}

void UISliderBar::update(Time t, bool moved)
{
	const auto size = getSize();
	const float thumbX = size.x * parent.getRelativeValue();

	left.setPos(getPosition() + Vector2f(-left.getUncroppedSize().x, 0));
	thumb.setPos(Vector2f(thumbX, 0) + getPosition());
	right.setPos(getPosition() + Vector2f(getSize().x, 0));
}

void UISliderBar::fill(UIPainter& painter, Rect4f rect, Sprite sprite) const
{
	auto p2 = painter.withClip(rect);
	const Vector2f spriteSize = sprite.getUncroppedSize();
	int n = int(ceil(getSize().x / spriteSize.x));
	for (int i = 0; i < n; ++i) {
		sprite.setPos(getPosition() + Vector2f(i * spriteSize.x, 0));
		p2.draw(sprite, true);
	}
}
