#include "select_asset_widget.h"

#include "halley/ui/ui_factory.h"
#include "halley/ui/widgets/ui_textinput.h"
#include "src/scene/choose_window.h"
#include "src/ui/project_window.h"
using namespace Halley;

SelectTargetWidget::SelectTargetWidget(const String& id, UIFactory& factory, IProjectWindow& projectWindow)
	: UIWidget(id, Vector2f(), UISizer(UISizerType::Horizontal))
	, factory(factory)
	, projectWindow(dynamic_cast<ProjectWindow&>(projectWindow))
	, allowEmpty("[None]")
	, aliveFlag(std::make_shared<bool>(true))
{
}

SelectTargetWidget::~SelectTargetWidget()
{
	*aliveFlag = false;
}

void SelectTargetWidget::update(Time t, bool moved)
{
	if (needFocus) {
		needFocus = false;
		focus();
	}
}

void SelectTargetWidget::setValue(const String& newValue)
{
	if (newValue != value || firstValue) {
		firstValue = false;
		value = newValue;
		onValueChanged(newValue);
		input->setText(getDisplayName());

		if (!displayErrorForEmpty || !defaultAssetId.isEmpty() || currentValueExists()) {
			input->getTextLabel().setColourOverride({});
		} else {
			input->getTextLabel().setColourOverride({ColourOverride(0, input->getStyles()[0].getColour("errorColour"))});
		}
		
		input->setIcon(makeIcon(), Vector4f(-2, 0, 2, 0));

		updateToolTip();
		notifyDataBind(value);
		sendEvent(UIEvent(UIEventType::TextChanged, getId(), value));
	}
}

String SelectTargetWidget::getValue() const
{
	return value;
}

void SelectTargetWidget::setDefaultAssetId(String assetId)
{
	defaultAssetId = std::move(assetId);
	if (input) {
		input->setGhostText(LocalisedString::fromUserString(getDisplayName(defaultAssetId)));
		updateToolTip();
	}
}

void SelectTargetWidget::setAllowEmpty(std::optional<String> allow)
{
	allowEmpty = std::move(allow);
}

void SelectTargetWidget::setDisplayErrorForEmpty(bool enabled)
{
	displayErrorForEmpty = enabled;
}

void SelectTargetWidget::makeUI()
{
	add(factory.makeUI("halley/select_asset_widget"), 1);

	getWidget("goto")->setActive(hasGoTo());

	input = getWidgetAs<UITextInput>("input");
	input->setIcon(makeIcon(), Vector4f(-2, 0, 2, 0));
	input->setReadOnly(true);
	input->setGhostText(LocalisedString::fromUserString(getDisplayName(defaultAssetId)));

	setHandle(UIEventType::ButtonClicked, "choose", [=] (const UIEvent& event)
	{
		choose();
	});

	setHandle(UIEventType::ButtonClicked, "goto", [=] (const UIEvent& event)
	{
		goToValue(event.getKeyMods());
	});
}

void SelectTargetWidget::choose()
{
	getRoot()->addChild(makeChooseWindow([=, flag = aliveFlag] (std::optional<String> result)
	{
		if (*flag && result) {
			setValue(result.value());
			needFocus = true;
		}
	}));
}

void SelectTargetWidget::updateToolTip()
{
	input->setToolTip(LocalisedString::fromUserString(doGetToolTip(value)));
}

void SelectTargetWidget::readFromDataBind()
{
	setValue(getDataBind()->getStringData());
}

String SelectTargetWidget::getDisplayName() const
{
	return getDisplayName(value);
}

String SelectTargetWidget::getDisplayName(const String& name) const
{
	if (name.isEmpty() && allowEmpty && defaultAssetId.isEmpty()) {
		return *allowEmpty;
	}
	return doGetDisplayName(name);
}

void SelectTargetWidget::goToValue(KeyMods keyMods)
{
}

bool SelectTargetWidget::hasGoTo() const
{
	return false;
}

bool SelectTargetWidget::currentValueExists()
{
	return true;
}

Sprite SelectTargetWidget::makeIcon()
{
	return Sprite();
}

String SelectTargetWidget::doGetDisplayName(const String& name) const
{
	return name;
}

String SelectTargetWidget::doGetToolTip(const String& value) const
{
	return value.isEmpty() ? defaultAssetId : value;
}

void SelectTargetWidget::onValueChanged(const String& value)
{
}

bool SelectTargetWidget::canReceiveFocus() const
{
	return true;
}


SelectAssetWidget::SelectAssetWidget(const String& id, UIFactory& factory, AssetType type, Resources& gameResources, IProjectWindow& projectWindow)
	: SelectTargetWidget(id, factory, projectWindow)
	, type(type)
	, gameResources(gameResources)
{
	makeUI();
}

std::shared_ptr<UIWidget> SelectAssetWidget::makeChooseWindow(std::function<void(std::optional<String>)> callback)
{
	if (type == AssetType::Prefab) {
		return std::make_shared<ChoosePrefabWindow>(factory, getValue(), gameResources, projectWindow, callback);
	} else {
		const bool preview = type == AssetType::Sprite || type == AssetType::Animation;
		return std::make_shared<ChooseAssetTypeWindow>(Vector2f(), factory, type, getValue(), gameResources, projectWindow, preview, allowEmpty, callback);
	}
}

void SelectAssetWidget::goToValue(KeyMods keyMods)
{
	auto uri = "asset:" + type + ":" + (value.isEmpty() ? defaultAssetId : value);
	const bool ctrlHeld = (static_cast<int>(keyMods) & static_cast<int>(KeyMods::Ctrl)) != 0;
	sendEvent(UIEvent(ctrlHeld ? UIEventType::NavigateToFile : UIEventType::NavigateToAsset, getId(), std::move(uri)));
}

bool SelectAssetWidget::hasGoTo() const
{
	return true;
}

bool SelectAssetWidget::currentValueExists()
{
	if (type == AssetType::Sprite && value.startsWith("$")) {
		return factory.getColourScheme()->hasSprite(value);
	}
	return gameResources.ofType(type).exists(value);
}

Sprite SelectAssetWidget::makeIcon()
{
	return factory.makeAssetTypeIcon(type);
}

String SelectAssetWidget::doGetDisplayName(const String& name) const
{
	if (type == AssetType::Sprite || type == AssetType::Animation || type == AssetType::MaterialDefinition) {
		return Path(name).getFilename().toString();
	} else {
		return name;
	}
}



SelectUIStyleWidget::SelectUIStyleWidget(const String& id, UIFactory& factory, String uiClass, Resources& gameResources, IProjectWindow& projectWindow)
	: SelectTargetWidget(id, factory, projectWindow)
	, uiClass(std::move(uiClass))
	, gameResources(gameResources)
{
	makeUI();
}

std::shared_ptr<UIWidget> SelectUIStyleWidget::makeChooseWindow(std::function<void(std::optional<String>)> callback)
{
	return std::make_shared<ChooseUIStyleWindow>(Vector2f(), factory, uiClass, "", gameResources, callback);
}

bool SelectUIStyleWidget::currentValueExists()
{
	return true;
}



SelectEntityWidget::SelectEntityWidget(const String& id, UIFactory& factory, IProjectWindow& projectWindow, IEntityEditorCallbacks* entityEditor)
	: SelectTargetWidget(id, factory, projectWindow)
	, entityEditor(entityEditor)
{
	makeUI();
}

std::shared_ptr<UIWidget> SelectEntityWidget::makeChooseWindow(std::function<void(std::optional<String>)> callback)
{
	return std::make_shared<ChooseEntityWindow>(factory, entityEditor ? entityEditor->getEntities() : Vector<IEntityEditorCallbacks::EntityInfo>(), std::move(callback));
}

void SelectEntityWidget::goToValue(KeyMods keyMods)
{
	if (entityEditor) {
		entityEditor->goToEntity(UUID(value));
	}
}

bool SelectEntityWidget::hasGoTo() const
{
	return !!entityEditor;
}

bool SelectEntityWidget::currentValueExists()
{
	return value.isEmpty() || !!info;
}

Sprite SelectEntityWidget::makeIcon()
{
	return info ? info->icon : Sprite();
}

String SelectEntityWidget::doGetDisplayName(const String& name) const
{
	return info ? info->name : name;
}

String SelectEntityWidget::doGetToolTip(const String& value) const
{
	return info ? info->name : value;
}

void SelectEntityWidget::onValueChanged(const String& value)
{
	info.reset();
	if (entityEditor && !value.isEmpty()) {
		const auto uuid = UUID(value);
		const auto newInfo = entityEditor->getEntityInfo(uuid);
		if (newInfo.uuid == uuid) {
			info = newInfo;
		}
	}
}
