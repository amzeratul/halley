#include "editor_ui_factory.h"
#include "halley/core/resources/resources.h"
#include <halley/file_formats/config_file.h>
#include "scroll_background.h"
#include "select_asset_widget.h"
#include "src/assets/animation_editor.h"
#include "src/assets/asset_editor_window.h"
#include "src/assets/metadata_editor.h"
#include "src/scene/entity_editor.h"
#include "src/scene/entity_list.h"
#include "src/scene/scene_editor_canvas.h"
using namespace Halley;

EditorUIFactory::EditorUIFactory(const HalleyAPI& api, Resources& resources, I18N& i18n, const String& colourSchemeName)
	: UIFactory(api, resources, i18n)
{
	loadColourSchemes();
	setColourScheme(colourSchemeName);
	
	UIInputButtons listButtons;
	setInputButtons("list", listButtons);

	addFactory("scrollBackground", [=] (const ConfigNode& node) { return makeScrollBackground(node); });
	addFactory("animationEditorDisplay", [=] (const ConfigNode& node) { return makeAnimationEditorDisplay(node); });
	addFactory("metadataEditor", [=] (const ConfigNode& node) { return makeMetadataEditor(node); });
	addFactory("sceneEditorCanvas", [=](const ConfigNode& node) { return makeSceneEditorCanvas(node); });
	addFactory("entityList", [=](const ConfigNode& node) { return makeEntityList(node); });
	addFactory("entityEditor", [=](const ConfigNode& node) { return makeEntityEditor(node); });
	addFactory("selectAsset", [=](const ConfigNode& node) { return makeSelectAsset(node); });
}

Sprite EditorUIFactory::makeAssetTypeIcon(AssetType type) const
{
	return Sprite()
		.setImage(getResources(), Path("ui") / "assetTypes" / toString(type) + ".png")
		.setColour(colourScheme->getColour("icon_" + toString(type)));
}

Sprite EditorUIFactory::makeImportAssetTypeIcon(ImportAssetType type) const
{
	return Sprite()
		.setImage(getResources(), Path("ui") / "assetTypes" / toString(type) + ".png")
		.setColour(colourScheme->getColour("icon_" + toString(type)));
}

Sprite EditorUIFactory::makeDirectoryIcon(bool up) const
{
	return Sprite()
		.setImage(getResources(), Path("ui") / "assetTypes" / (up ? "directoryUp" : "directory") + ".png")
		.setColour(colourScheme->getColour("icon_directory"));
}

std::shared_ptr<UIWidget> EditorUIFactory::makeScrollBackground(const ConfigNode& entryNode)
{
	return std::make_shared<ScrollBackground>("scrollBackground", resources, makeSizerOrDefault(entryNode, UISizer(UISizerType::Vertical)));
}

std::shared_ptr<UIWidget> EditorUIFactory::makeAnimationEditorDisplay(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	return std::make_shared<AnimationEditorDisplay>(id, resources);
}

std::shared_ptr<UIWidget> EditorUIFactory::makeMetadataEditor(const ConfigNode& entryNode)
{
	return std::make_shared<MetadataEditor>(*this);
}

std::shared_ptr<UIWidget> EditorUIFactory::makeSceneEditorCanvas(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	return std::make_shared<SceneEditorCanvas>(id, *this, resources, api, makeSizer(entryNode));
}

std::shared_ptr<UIWidget> EditorUIFactory::makeEntityList(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	return std::make_shared<EntityList>(id, *this);
}

std::shared_ptr<UIWidget> EditorUIFactory::makeEntityEditor(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	return std::make_shared<EntityEditor>(id, *this);
}

std::shared_ptr<UIWidget> EditorUIFactory::makeSelectAsset(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	return std::make_shared<SelectAssetWidget>(id, *this, fromString<AssetType>(node["assetType"].asString()));
}

void EditorUIFactory::loadColourSchemes()
{
	for (const auto& assetId: resources.enumerate<ConfigFile>()) {
		if (assetId.startsWith("colour_schemes/")) {
			auto scheme = std::make_shared<UIColourScheme>(resources.get<ConfigFile>(assetId)->getRoot(), resources);
			if (scheme->isEnabled()) {
				colourSchemes.push_back(scheme);
			}
		}
	}
}

std::vector<String> EditorUIFactory::getColourSchemeNames() const
{
	std::vector<String> result;
	for (auto& scheme: colourSchemes) {
		result.push_back(scheme->getName());
	}
	return result;
}

void EditorUIFactory::setColourScheme(const String& name)
{
	bool found = false;

	for (auto& scheme: colourSchemes) {
		if (scheme->getName() == name) {
			colourScheme = scheme;
			found = true;
			break;
		}
	}

	if (!found && !colourSchemes.empty()) {
		colourScheme = colourSchemes.front();
		found = true;
	}

	if (found) {
		reloadStyleSheet();
	}
}

void EditorUIFactory::reloadStyleSheet()
{
	const auto cur = getStyleSheet();
	
	if (true || !cur) {
		auto styleSheet = std::make_shared<UIStyleSheet>(resources);
		for (auto& style: resources.enumerate<ConfigFile>()) {
			if (style.startsWith("ui_style/")) {
				styleSheet->load(*resources.get<ConfigFile>(style), colourScheme);
			}
		}
		setStyleSheet(std::move(styleSheet));
	} else {
		// This doesn't work properly atm
		cur->reload(colourScheme);
	}
}
