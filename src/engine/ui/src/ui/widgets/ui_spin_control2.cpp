#include "halley/ui/widgets/ui_spin_control2.h"
#include "halley/ui/widgets/ui_button.h"
#include "halley/ui/ui_validator.h"
using namespace Halley;

UISpinControl2::UISpinControl2(String id, UIStyle style, float value, bool allowFloat)
	: UITextInput(std::move(id), style, toString(value), {}, std::make_shared<UINumericValidator>(true, allowFloat))
{
	const auto upButton = std::make_shared<UIButton>(id + "_up", style.getSubStyle("spinButton"), UISizer());
	const auto downButton = std::make_shared<UIButton>(id + "_down", style.getSubStyle("spinButton"), UISizer());
	upButton->setIcon(style.getSprite("upButton"));
	upButton->setMinSize(Vector2f(18, 9));
	downButton->setIcon(style.getSprite("downButton"));
	downButton->setMinSize(Vector2f(18, 9));

	auto sizer = std::make_shared<UISizer>(UISizerType::Vertical, 0.0f);
	sizer->add(upButton, 1);
	sizer->add(downButton, 1);
	UIWidget::add(std::move(sizer), 0, Vector4f(0, -1, -5, -2), UISizerAlignFlags::Right);

	setHandle(UIEventType::ButtonClicked, id + "_down", [=] (const UIEvent& event)
	{
		setValue(getValue() - increment);
	});
	setHandle(UIEventType::ButtonClicked, id + "_up", [=] (const UIEvent& event)
	{
		setValue(getValue() + increment);
	});
}

void UISpinControl2::setValue(float value)
{
	setText(toString(value));
}

float UISpinControl2::getValue() const
{
	return getText().toFloat();
}

void UISpinControl2::setIncrement(float inc)
{
	increment = inc;
}

void UISpinControl2::setMinimumValue(std::optional<float> value)
{
	minValue = value;
}

void UISpinControl2::setMaximumValue(std::optional<float> value)
{
	maxValue = value;
}

void UISpinControl2::onManualControlCycleValue(int delta)
{
	setValue(getValue() + delta * increment);
}

void UISpinControl2::readFromDataBind()
{
	if (getDataBind()->getFormat() == UIDataBind::Format::Float) {
		setValue(getDataBind()->getFloatData());
	} else {
		setValue(static_cast<float>(getDataBind()->getIntData()));
	}
}

Vector4f UISpinControl2::getTextInnerBorder() const
{
	auto value = UITextInput::getTextInnerBorder();
	value.z += 18;
	return value;
}
