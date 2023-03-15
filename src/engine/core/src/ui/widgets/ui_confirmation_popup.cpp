#include "halley/ui/widgets/ui_confirmation_popup.h"
#include "halley/ui/ui_anchor.h"
#include "halley/ui/ui_factory.h"
#include "halley/input/input_keyboard.h"
#include "halley/utils/algorithm.h"
#include "halley/ui/widgets/ui_label.h"
using namespace Halley;

UIConfirmationPopup::UIConfirmationPopup(UIFactory& factory, String title, String message, Vector<ButtonType> buttons, Callback callback)
	: UIWidget("confirmationPopup", Vector2f(), UISizer())
	, title(std::move(title))
	, message(std::move(message))
	, buttons(std::move(buttons))
	, callback(std::move(callback))
{
	factory.loadUI(*this, "halley/confirmation_popup");

	setChildLayerAdjustment(100);
	setModal(true);
	setMouseBlocker(true);
	setAnchor(UIAnchor());
}

void UIConfirmationPopup::onAddedToRoot(UIRoot& root)
{
	root.registerKeyPressListener(shared_from_this(), 10);
	focus();
}

void UIConfirmationPopup::onRemovedFromRoot(UIRoot& root)
{
	root.removeKeyPressListener(*this);
}

void UIConfirmationPopup::onMakeUI()
{
	getWidgetAs<UILabel>("title")->setText(LocalisedString::fromUserString(title));
	getWidgetAs<UILabel>("message")->setText(LocalisedString::fromUserString(message));

	for (auto button: { ButtonType::Yes, ButtonType::No, ButtonType::Ok, ButtonType::Cancel }) {
		getWidget(toString(button))->setActive(std_ex::contains(buttons, button));
	}

	setHandle(UIEventType::ButtonClicked, [=] (const UIEvent& event)
	{
		const auto button = fromString<ButtonType>(event.getSourceId());
		callback(button);
		destroy();
	});
}

bool UIConfirmationPopup::onKeyPress(KeyboardKeyPress key)
{
	if (key.is(KeyCode::Enter)) {
		for (const auto& button: buttons) {
			if (button == ButtonType::Ok || button == ButtonType::Yes) {
				callback(button);
				destroy();
				return true;
			}
		}
	}

	if (key.is(KeyCode::Esc)) {
		for (const auto& button: buttons) {
			if (button == ButtonType::Cancel || button == ButtonType::No) {
				callback(button);
				destroy();
				return true;
			}
		}
	}

	return false;
}
