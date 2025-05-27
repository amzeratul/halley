#include "halley/ui/widgets/ui_input_popup.h"
#include "halley/ui/ui_anchor.h"
#include "halley/ui/ui_factory.h"
#include "halley/input/input_keyboard.h"
#include "halley/utils/algorithm.h"
#include "halley/ui/widgets/ui_label.h"
using namespace Halley;

UIInputPopup::UIInputPopup(UIFactory& factory, String title, String message, String defaultValue, Callback callback)
	: UIWidget("confirmationPopup", Vector2f(), UISizer())
	, title(std::move(title))
	, message(std::move(message))
	, defaultValue(std::move(defaultValue))
	, callback(std::move(callback))
{
	factory.loadUI(*this, "halley/input_popup");

	setChildLayerAdjustment(100);
	setModal(true);
	setMouseBlocker(true);
	setAnchor(UIAnchor());
}

void UIInputPopup::onAddedToRoot(UIRoot& root)
{
	root.registerKeyPressListener(shared_from_this(), 10);
}

void UIInputPopup::onRemovedFromRoot(UIRoot& root)
{
	root.removeKeyPressListener(*this);
}

void UIInputPopup::onMakeUI()
{
	getWidgetAs<UILabel>("title")->setText(LocalisedString::fromUserString(title));
	getWidgetAs<UILabel>("message")->setText(LocalisedString::fromUserString(message));

	getWidgetAs<UITextInput>("input")->setText(defaultValue);

	setHandle(UIEventType::ButtonClicked, [=] (const UIEvent& event)
	{
		onResult(event.getSourceId() == "ok");
	});
}

bool UIInputPopup::onKeyPress(KeyboardKeyPress key)
{
	if (key.is(KeyCode::Enter)) {
		onResult(true);
		return true;
	}

	if (key.is(KeyCode::Esc)) {
		onResult(false);
		return true;
	}

	return false;
}

void UIInputPopup::onResult(bool ok)
{
	if (ok) {
		callback(getWidgetAs<UITextInput>("input")->getText());
	} else {
		callback(std::nullopt);
	}
	destroy();
}

void UIInputPopup::update(Time t, bool moved)
{
	if (needFocus) {
		getWidget("input")->focus();
		needFocus = false;
	}
}
