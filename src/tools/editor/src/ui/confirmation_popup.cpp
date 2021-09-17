#include "confirmation_popup.h"
using namespace Halley;

ConfirmationPopup::ConfirmationPopup(UIFactory& factory, String title, String message, std::vector<ButtonType> buttons, Callback callback)
	: UIWidget("confirmationPopup", Vector2f(), UISizer())
	, title(std::move(title))
	, message(std::move(message))
	, buttons(std::move(buttons))
	, callback(std::move(callback))
{
	factory.loadUI(*this, "ui/halley/confirmation_popup");

	setChildLayerAdjustment(100);
	setModal(true);
	setMouseBlocker(true);
	setAnchor(UIAnchor());
}

void ConfirmationPopup::onMakeUI()
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
