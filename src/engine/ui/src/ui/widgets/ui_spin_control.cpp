#include "halley/ui/widgets/ui_spin_control.h"
#include "halley/ui/widgets/ui_button.h"
#include "halley/ui/ui_validator.h"
using namespace Halley;

UISpinControl::UISpinControl(std::shared_ptr<InputKeyboard> keyboard, String id, UIStyle style, int value)
	: UIWidget(id, {}, UISizer())
	, value(value)
{
	textInput = std::make_shared<UITextInput>(keyboard, id + "_textinput", style.getSubStyle("input"), toString(value));
	textInput->setValidator(std::make_shared<UINumericValidator>(true));

	const auto leftButton = std::make_shared<UIButton>(id + "_left", style.getSubStyle("leftButton"));
	const auto rightButton = std::make_shared<UIButton>(id + "_right", style.getSubStyle("rightButton"));
	UIWidget::add(leftButton, 0, Vector4f(), UISizerAlignFlags::Centre);
	UIWidget::add(textInput, 1);
	UIWidget::add(rightButton, 0, Vector4f(), UISizerAlignFlags::Centre);

	setHandle(UIEventType::ButtonClicked, id + "_left", [=] (const UIEvent& event)
	{
		setValue(getValue() - 1);
	});
	setHandle(UIEventType::ButtonClicked, id + "_right", [=] (const UIEvent& event)
	{
		setValue(getValue() + 1);
	});
	setHandle(UIEventType::TextChanged, id + "_textinput", [=] (const UIEvent& event)
	{
		setValue(event.getData().toInteger());
	});
}

void UISpinControl::setValue(int v)
{
	int finalValue = v;
	if (minValue) {
		finalValue = std::max(finalValue, int(*minValue));
	}
	if (maxValue) {
		finalValue = std::max(finalValue, int(*maxValue));
	}

	if (value != v) {
		value = v;
		textInput->setText(toString(v));
		notifyDataBind(value);
	}
}

int UISpinControl::getValue() const
{
	return value;
}

void UISpinControl::setMinimumValue(Maybe<int> value)
{
	minValue = value;
}

void UISpinControl::setMaximumValue(Maybe<int> value)
{
	maxValue = value;
}

void UISpinControl::onManualControlActivate()
{
	textInput->onManualControlActivate();
}

void UISpinControl::onManualControlCycleValue(int delta)
{
	setValue(getValue() + delta);
}

void UISpinControl::readFromDataBind()
{
	setValue(getDataBind()->getIntData());
}
