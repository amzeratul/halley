#include "halley/ui/widgets/ui_goto_popup.h"

#include "halley/ui/ui_anchor.h"
#include "halley/ui/ui_factory.h"
#include "halley/input/input_keyboard.h"
#include "halley/ui/widgets/ui_spin_control2.h"
using namespace Halley;

UIGoToPopup::UIGoToPopup(UIFactory& factory, Vector2f startValue, Callback callback)
	: UIWidget("goto_popup", {}, UISizer())
	, startValue(startValue)
	, callback(std::move(callback))
{
	factory.loadUI(*this, "halley/goto_popup");
	setAnchor(UIAnchor());
}

void UIGoToPopup::onAddedToRoot(UIRoot& root)
{
	root.registerKeyPressListener(shared_from_this(), 10);
	root.focusNext(false);
}

void UIGoToPopup::onRemovedFromRoot(UIRoot& root)
{
	root.removeKeyPressListener(*this);
}

void UIGoToPopup::onMakeUI()
{
	getWidgetAs<UISpinControl2>("posX")->setValue(startValue.x);
	getWidgetAs<UISpinControl2>("posY")->setValue(startValue.y);

	setHandle(UIEventType::ButtonClicked, "ok", [=](const UIEvent& event)
	{
		onOK();
	});
	
	setHandle(UIEventType::ButtonClicked, "cancel", [=](const UIEvent& event)
	{
		onCancel();
	});

	setHandle(UIEventType::TextSubmit, "posX", [=](const UIEvent& event)
	{
		onOK();
	});

	setHandle(UIEventType::TextSubmit, "posY", [=](const UIEvent& event)
	{
		onOK();
	});
}

bool UIGoToPopup::onKeyPress(KeyboardKeyPress key)
{
	if (key.is(KeyCode::Enter)) {
		onOK();
	}

	if (key.is(KeyCode::Esc)) {
		onCancel();
	}

	return false;
}

void UIGoToPopup::onOK()
{
	Vector2f value;
	value.x = getWidgetAs<UISpinControl2>("posX")->getValue();
	value.y = getWidgetAs<UISpinControl2>("posY")->getValue();
	callback(value);
	destroy();
}

void UIGoToPopup::onCancel()
{
	callback(std::nullopt);
	destroy();
}
