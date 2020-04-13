#include "select_asset_widget.h"

#include "src/scene/choose_asset_window.h"
using namespace Halley;

SelectAssetWidget::SelectAssetWidget(const String& id, UIFactory& factory, AssetType type, Resources& resources)
	: UIWidget(id, Vector2f(), UISizer(UISizerType::Horizontal))
	, factory(factory)
	, resources(resources)
	, type(type)
{
	makeUI();
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
	getRoot()->addChild(std::make_shared<ChooseAssetTypeWindow>(factory, type, input->getText(), resources, [=] (std::optional<String> result)
	{
		if (result) {
			input->setText(result.value());
			notifyDataBind(result.value());
		}
	}));
}

void SelectAssetWidget::readFromDataBind()
{
	input->setText(getDataBind()->getStringData());
}
