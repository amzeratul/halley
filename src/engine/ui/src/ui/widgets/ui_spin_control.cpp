#include "halley/ui/widgets/ui_spin_control.h"
#include "halley/ui/widgets/ui_button.h"
#include "halley/ui/ui_validator.h"
#include "halley/support/logger.h"
using namespace Halley;

UISpinControl::UISpinControl(std::shared_ptr<InputKeyboard> keyboard, String id, UIStyle style, float value)
	: UIWidget(id, {}, UISizer())
	, value(99999)
{
	textInput = std::make_shared<UITextInput>(keyboard, id + "_textinput", style.getSubStyle("input"), "99999");
	textInput->setValidator(std::make_shared<UINumericValidator>(true, false));

	const auto leftButton = std::make_shared<UIButton>(id + "_left", style.getSubStyle("leftButton"));
	const auto rightButton = std::make_shared<UIButton>(id + "_right", style.getSubStyle("rightButton"));
	UIWidget::add(leftButton, 0, Vector4f(), UISizerAlignFlags::Centre);
	UIWidget::add(textInput, 1);
	UIWidget::add(rightButton, 0, Vector4f(), UISizerAlignFlags::Centre);

	setHandle(UIEventType::ButtonClicked, id + "_left", [=] (const UIEvent& event)
	{
		setValue(getValue() - increment);
	});
	setHandle(UIEventType::ButtonClicked, id + "_right", [=] (const UIEvent& event)
	{
		setValue(getValue() + increment);
	});

	setHandle(UIEventType::FocusLost, id + "_textinput", [=] (const UIEvent& event)
	{
		setValue(textInput->getText().toFloat());
	});
	setHandle(UIEventType::TextSubmit, id + "_textinput", [=] (const UIEvent& event)
	{
		setValue(textInput->getText().toFloat());
	});

	setValue(value);
}

void UISpinControl::setValue(float v)
{
	float finalValue = lround(v / increment) * increment;

	if (minValue) {
		finalValue = std::max(finalValue, float(*minValue));
	}
	if (maxValue) {
		finalValue = std::min(finalValue, float(*maxValue));
	}

	const bool changed = value != finalValue;
	if (changed) {
		value = finalValue;
	}

	if (changed || finalValue != v) {
		textInput->setCanSendEvents(false);
		textInput->setText(toString(finalValue));
		textInput->setCanSendEvents(true);
	}

	if (changed) {
		notifyDataBind(value);
	}
}

float UISpinControl::getValue() const
{
	return value;
}

void UISpinControl::setIncrement(float inc)
{
	increment = inc;

	const bool isFloat = std::fabs(std::fmod(inc, 1.0f)) > 0.0001f;
	textInput->setValidator(std::make_shared<UINumericValidator>(true, isFloat));
}

void UISpinControl::setMinimumValue(std::optional<float> value)
{
	minValue = value;
}

void UISpinControl::setMaximumValue(std::optional<float> value)
{
	maxValue = value;
}

void UISpinControl::onManualControlActivate()
{
	textInput->onManualControlActivate();
}

void UISpinControl::onManualControlCycleValue(int delta)
{
	setValue(getValue() + delta * increment);
}

void UISpinControl::readFromDataBind()
{
	if (getDataBind()->getFormat() == UIDataBind::Format::Float) {
		setValue(getDataBind()->getFloatData());
	} else {
		setValue(float(getDataBind()->getIntData()));
	}
}
