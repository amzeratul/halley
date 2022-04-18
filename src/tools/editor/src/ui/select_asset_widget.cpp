#include "select_asset_widget.h"

#include "halley/ui/ui_factory.h"
#include "halley/ui/widgets/ui_textinput.h"
#include "src/scene/choose_window.h"
#include "src/ui/project_window.h"
using namespace Halley;

SelectAssetWidget::SelectAssetWidget(const String& id, UIFactory& factory, AssetType type, Resources& gameResources, IProjectWindow& projectWindow)
	: UIWidget(id, Vector2f(), UISizer(UISizerType::Horizontal))
	, factory(factory)
	, gameResources(gameResources)
	, projectWindow(dynamic_cast<ProjectWindow&>(projectWindow))
	, type(type)
	, allowEmpty("[None]")
{
	makeUI();
}

void SelectAssetWidget::setValue(const String& newValue)
{
	if (newValue != value || firstValue) {
		firstValue = false;
		value = newValue;
		input->setText(getDisplayName());

		if (!displayErrorForEmpty || gameResources.ofType(type).exists(newValue)) {
			input->getTextLabel().setColourOverride({});
		} else {
			input->getTextLabel().setColourOverride({ColourOverride(0, input->getStyles()[0].getColour("errorColour"))});
		}
		
		updateToolTip();
		notifyDataBind(value);
		sendEvent(UIEvent(UIEventType::TextChanged, getId(), value));
	}
}

String SelectAssetWidget::getValue() const
{
	return value;
}

void SelectAssetWidget::setDefaultAssetId(String assetId)
{
	defaultAssetId = std::move(assetId);
	if (input) {
		input->setGhostText(LocalisedString::fromUserString(getDisplayName(defaultAssetId)));
		updateToolTip();
	}
}

void SelectAssetWidget::setAllowEmpty(std::optional<String> allow)
{
	allowEmpty = std::move(allow);
}

void SelectAssetWidget::setDisplayErrorForEmpty(bool enabled)
{
	displayErrorForEmpty = enabled;
}

void SelectAssetWidget::makeUI()
{
	add(factory.makeUI("halley/select_asset_widget"), 1);

	input = getWidgetAs<UITextInput>("input");
	input->setIcon(factory.makeAssetTypeIcon(type), Vector4f(-2, 0, 2, 0));
	input->setReadOnly(true);
	input->setGhostText(LocalisedString::fromUserString(getDisplayName(defaultAssetId)));

	setHandle(UIEventType::ButtonClicked, "choose", [=] (const UIEvent& event)
	{
		choose();
	});

	setHandle(UIEventType::ButtonClicked, "goto", [=] (const UIEvent& event)
	{
		auto uri = "asset:" + type + ":" + (value.isEmpty() ? defaultAssetId : value);
		const bool ctrlHeld = (static_cast<int>(event.getKeyMods()) & static_cast<int>(KeyMods::Ctrl)) != 0;
		sendEvent(UIEvent(ctrlHeld ? UIEventType::NavigateToFile : UIEventType::NavigateToAsset, getId(), std::move(uri)));
	});
}

void SelectAssetWidget::choose()
{
	auto callback = [=] (std::optional<String> result)
	{
		if (result) {
			setValue(result.value());
		}
	};

	std::shared_ptr<UIWidget> window;
	if (type == AssetType::Prefab) {
		assert(projectWindow != nullptr);
		window = std::make_shared<ChoosePrefabWindow>(factory, getValue(), gameResources, projectWindow, callback);
	} else {
		const bool preview = type == AssetType::Sprite || type == AssetType::Animation;
		window = std::make_shared<ChooseAssetTypeWindow>(Vector2f(), factory, type, getValue(), gameResources, projectWindow, preview, allowEmpty, callback);
	}
	getRoot()->addChild(std::move(window));
}

void SelectAssetWidget::updateToolTip()
{
	input->setToolTip(LocalisedString::fromUserString(value.isEmpty() ? defaultAssetId : value));
}

void SelectAssetWidget::readFromDataBind()
{
	setValue(getDataBind()->getStringData());
}

String SelectAssetWidget::getDisplayName() const
{
	return getDisplayName(value);
}

String SelectAssetWidget::getDisplayName(const String& name) const
{
	if (name.isEmpty() && allowEmpty) {
		return *allowEmpty;
	}
	if (type == AssetType::Sprite || type == AssetType::Animation || type == AssetType::MaterialDefinition) {
		return Path(name).getFilename().toString();
	} else {
		return name;
	}
}
