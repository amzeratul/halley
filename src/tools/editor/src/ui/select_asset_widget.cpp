#include "select_asset_widget.h"

#include "halley/ui/ui_factory.h"
#include "halley/ui/widgets/ui_textinput.h"
#include "src/scene/choose_asset_window.h"
using namespace Halley;

SelectAssetWidget::SelectAssetWidget(const String& id, UIFactory& factory, AssetType type)
	: UIWidget(id, Vector2f(), UISizer(UISizerType::Horizontal))
	, factory(factory)
	, type(type)
{
	makeUI();
}

SelectAssetWidget::SelectAssetWidget(const String& id, UIFactory& factory, AssetType type, Resources& gameResources)
	: SelectAssetWidget(id, factory, type)
{
	this->gameResources = &gameResources;
}

void SelectAssetWidget::setValue(const String& value)
{
	if (value != getValue()) {
		input->setText(value);
		notifyDataBind(value);
		sendEvent(UIEvent(UIEventType::TextChanged, getId(), value));
	}
}

String SelectAssetWidget::getValue() const
{
	return input->getText();
}

void SelectAssetWidget::setGameResources(Resources& resources)
{
	gameResources = &resources;
}

void SelectAssetWidget::makeUI()
{
	add(factory.makeUI("ui/halley/select_asset_widget"), 1);

	input = getWidgetAs<UITextInput>("input");
	input->setReadOnly(true);

	setHandle(UIEventType::ButtonClicked, "choose", [=] (const UIEvent& event)
	{
		choose();
	});
}

void SelectAssetWidget::choose()
{
	if (gameResources) {
		getRoot()->addChild(std::make_shared<ChooseAssetTypeWindow>(factory, type, input->getText(), *gameResources, [=] (std::optional<String> result)
		{
			if (result) {
				setValue(result.value());
			}
		}));
	}
}

void SelectAssetWidget::readFromDataBind()
{
	input->setText(getDataBind()->getStringData());
}
