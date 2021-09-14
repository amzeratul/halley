#include "select_asset_widget.h"

#include "halley/ui/ui_factory.h"
#include "halley/ui/widgets/ui_textinput.h"
#include "src/scene/choose_asset_window.h"
#include "src/scene/scene_editor_window.h"
using namespace Halley;

SelectAssetWidget::SelectAssetWidget(const String& id, UIFactory& factory, AssetType type)
	: UIWidget(id, Vector2f(), UISizer(UISizerType::Horizontal))
	, factory(factory)
	, type(type)
{
	makeUI();
}

SelectAssetWidget::SelectAssetWidget(const String& id, UIFactory& factory, AssetType type, Resources& gameResources, ISceneEditorWindow& sceneEditorWindow)
	: SelectAssetWidget(id, factory, type)
{
	this->gameResources = &gameResources;
	this->sceneEditorWindow = &dynamic_cast<SceneEditorWindow&>(sceneEditorWindow);
}

void SelectAssetWidget::setValue(const String& newValue)
{
	if (newValue != value) {
		value = newValue;
		input->setText(getDisplayName());

		if (gameResources->ofType(type).exists(newValue)) {
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

void SelectAssetWidget::setGameResources(Resources& resources)
{
	gameResources = &resources;
}

void SelectAssetWidget::setDefaultAssetId(String assetId)
{
	defaultAssetId = std::move(assetId);
	if (input) {
		input->setGhostText(LocalisedString::fromUserString(getDisplayName(defaultAssetId)));
		updateToolTip();
	}
}

void SelectAssetWidget::setSceneEditorWindow(SceneEditorWindow& window)
{
	sceneEditorWindow = &window;
}

void SelectAssetWidget::makeUI()
{
	add(factory.makeUI("ui/halley/select_asset_widget"), 1);

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
		sendEvent(UIEvent(UIEventType::NavigateTo, getId(), std::move(uri)));
	});
}

void SelectAssetWidget::choose()
{
	if (gameResources) {
		auto callback = [=] (std::optional<String> result)
		{
			if (result) {
				setValue(result.value());
			}
		};

		std::shared_ptr<UIWidget> window;
		if (type == AssetType::Prefab) {
			assert(sceneEditorWindow != nullptr);
			window = std::make_shared<ChoosePrefabWindow>(factory, getValue(), *gameResources, *sceneEditorWindow, callback);
		} else {
			const bool preview = type == AssetType::Sprite || type == AssetType::Animation;
			window = std::make_shared<ChooseAssetTypeWindow>(factory, type, getValue(), *gameResources, *sceneEditorWindow, preview, callback);
		}
		getRoot()->addChild(std::move(window));
	}
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
	if (type == AssetType::Sprite || type == AssetType::Animation || type == AssetType::MaterialDefinition) {
		return Path(name).getFilename().toString();
	} else {
		return name;
	}
}
