#include "editor_ui_factory.h"
#include "halley/resources/resources.h"
#include <halley/file_formats/config_file.h>

#include "colour_picker.h"
#include "infini_canvas.h"
#include "scroll_background.h"
#include "select_asset_widget.h"
#include "src/assets/animation_editor.h"
#include "src/assets/asset_editor_window.h"
#include "src/assets/curve_editor.h"
#include "src/assets/gradient_editor.h"
#include "src/assets/audio_editor/audio_object_editor.h"
#include "src/assets/metadata_editor.h"
#include "src/assets/ui_editor/ui_editor_display.h"
#include "src/assets/ui_editor/ui_widget_editor.h"
#include "src/assets/ui_editor/ui_widget_list.h"
#include "src/scene/entity_editor.h"
#include "src/scene/entity_list.h"
#include "src/scene/entity_validator_ui.h"
#include "src/scene/scene_editor_canvas.h"
#include "src/assets/graph/script_graph_variable_inspector.h"
using namespace Halley;

EditorUIFactory::EditorUIFactory(const HalleyAPI& api, Resources& resources, I18N& i18n, const String& colourSchemeName)
	: UIFactory(api, resources, i18n)
{
	loadColourSchemes();
	setColourScheme(colourSchemeName);
	
	UIInputButtons listButtons;
	setInputButtons("list", listButtons);
	setInputButtons("treeList", listButtons);

	addFactory("scrollBackground", [=] (const ConfigNode& node) { return makeScrollBackground(node); });
	addFactory("infiniCanvas", [=] (const ConfigNode& node) { return makeInfiniCanvas(node); });
	addFactory("animationEditorDisplay", [=] (const ConfigNode& node) { return makeAnimationEditorDisplay(node); });
	addFactory("metadataEditor", [=] (const ConfigNode& node) { return makeMetadataEditor(node); });
	addFactory("sceneEditorCanvas", [=](const ConfigNode& node) { return makeSceneEditorCanvas(node); });
	addFactory("entityList", [=](const ConfigNode& node) { return makeEntityList(node); });
	addFactory("entityValidator", [=](const ConfigNode& node) { return makeEntityValidator(node); });
	addFactory("entityValidatorList", [=](const ConfigNode& node) { return makeEntityValidatorList(node); });
	addFactory("entityEditor", [=](const ConfigNode& node) { return makeEntityEditor(node); });
	addFactory("selectAsset", [=](const ConfigNode& node) { return makeSelectAsset(node); });
	addFactory("uiWidgetList", [=](const ConfigNode& node) { return makeUIWidgetList(node); });
	addFactory("uiWidgetEditor", [=](const ConfigNode& node) { return makeUIWidgetEditor(node); });
	addFactory("uiEditorDisplay", [=](const ConfigNode& node) { return makeUIEditorDisplay(node); });
	addFactory("audioObjectTreeList", [=](const ConfigNode& node) { return makeAudioObjectTreeList(node); });
	addFactory("gradientEditor", [=](const ConfigNode& node) { return makeGradientEditor(node); });
	addFactory("curveEditor", [=](const ConfigNode& node) { return makeCurveEditor(node); });
	addFactory("colourPickerDisplay", [=](const ConfigNode& node) { return makeColourPickerDisplay(node); });
	addFactory("scriptingVariableInspector", [=](const ConfigNode& node) { return makeScriptingVariableInspector(node);  });
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
	auto& node = entryNode["widget"];
	return std::make_shared<ScrollBackground>("scrollBackground", getStyle(node["style"].asString("scrollBackground")), makeSizerOrDefault(entryNode, UISizer(UISizerType::Vertical)), api.input->getKeyboard());
}

std::shared_ptr<UIWidget> EditorUIFactory::makeInfiniCanvas(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	return std::make_shared<InfiniCanvas>("infiniCanvas", getStyle(node["style"].asString("infiniCanvas")), makeSizerOrDefault(entryNode, UISizer(UISizerType::Vertical)), api.input->getKeyboard());
}

std::shared_ptr<UIWidget> EditorUIFactory::makeAnimationEditorDisplay(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	return std::make_shared<AnimationEditorDisplay>(id, resources);
}

std::shared_ptr<UIWidget> EditorUIFactory::makeMetadataEditor(const ConfigNode& entryNode)
{
	Expects(projectWindow != nullptr);
	return std::make_shared<MetadataEditor>(*this, *projectWindow);
}

std::shared_ptr<UIWidget> EditorUIFactory::makeSceneEditorCanvas(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	return std::make_shared<SceneEditorCanvas>(id, *this, resources, api, projectWindow->getProject(), makeSizer(entryNode));
}

std::shared_ptr<UIWidget> EditorUIFactory::makeEntityList(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	return std::make_shared<EntityList>(id, *this);
}

std::shared_ptr<UIWidget> EditorUIFactory::makeEntityValidator(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	return std::make_shared<EntityValidatorUI>(id, *this);
}

std::shared_ptr<UIWidget> EditorUIFactory::makeEntityValidatorList(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	return std::make_shared<EntityValidatorListUI>(id, *this);
}

std::shared_ptr<UIWidget> EditorUIFactory::makeEntityEditor(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	return std::make_shared<EntityEditor>(id, *this);
}

std::shared_ptr<UIWidget> EditorUIFactory::makeSelectAsset(const ConfigNode& entryNode)
{
	Expects(gameResources != nullptr);
	Expects(projectWindow != nullptr);
	
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	const auto assetType = fromString<AssetType>(node["assetType"].asString());

	auto widget = std::make_shared<SelectAssetWidget>(id, *this, assetType, *gameResources, *projectWindow);
	if (node.hasKey("allowEmpty")) {
		widget->setAllowEmpty(node["allowEmpty"].asString(""));
	}
	widget->setDisplayErrorForEmpty(node["displayErrorForEmpty"].asBool(true));
	return widget;
}

std::shared_ptr<UIWidget> EditorUIFactory::makeUIWidgetEditor(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	return std::make_shared<UIWidgetEditor>(id, *this);
}

std::shared_ptr<UIWidget> EditorUIFactory::makeUIWidgetList(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	return std::make_shared<UIWidgetList>(id, *this);
}

std::shared_ptr<UIWidget> EditorUIFactory::makeUIEditorDisplay(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	return std::make_shared<UIEditorDisplay>(id, Vector2f{}, makeSizer(entryNode).value_or(UISizer()), api, resources);
}

std::shared_ptr<UIWidget> EditorUIFactory::makeAudioObjectTreeList(const ConfigNode& entryNode)
{
	const auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("treeList"), getStyleSheet());
	auto label = parseLabel(node);

	auto widget = std::make_shared<AudioObjectEditorTreeList>(id, style);
	applyListProperties(*widget, node, "treeList");

	return widget;
}

std::shared_ptr<UIWidget> EditorUIFactory::makeCurveEditor(const ConfigNode& entryNode)
{
	const auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("curveEditor"), getStyleSheet());

	auto widget = std::make_shared<CurveEditor>(*this, id, style);

	return widget;
}

std::shared_ptr<UIWidget> EditorUIFactory::makeGradientEditor(const ConfigNode& entryNode)
{
	const auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("gradientEditor"), getStyleSheet());

	auto widget = std::make_shared<GradientEditor>(*this, id, style, *api.video);

	return widget;
}

std::shared_ptr<UIWidget> EditorUIFactory::makeColourPickerDisplay(const ConfigNode& entryNode)
{
	const auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto size = node["size"].asVector2f(Vector2f(16, 16));
	auto material = node["material"].asString();
	auto widget = std::make_shared<ColourPickerDisplay>(std::move(id), size, getResources(), material);

	return widget;	
}

std::shared_ptr<UIWidget> EditorUIFactory::makeScriptingVariableInspector(const ConfigNode& entryNode)
{
	const auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto widget = std::make_shared<ScriptGraphVariableInspector>(*this);
	return widget;
}

void EditorUIFactory::loadColourSchemes()
{
	for (const auto& assetId: resources.enumerate<ConfigFile>()) {
		if (assetId.startsWith("colour_schemes/")) {
			auto& schemeConfig = resources.get<ConfigFile>(assetId)->getRoot();
			if (schemeConfig["enabled"].asBool(true)) {
				colourSchemes.emplace_back(assetId, schemeConfig["name"].asString());
			}
		}
	}
}

Vector<String> EditorUIFactory::getColourSchemeNames() const
{
	Vector<String> result;
	for (const auto& scheme: colourSchemes) {
		result.push_back(scheme.second);
	}
	std::sort(result.begin(), result.end());
	return result;
}

void EditorUIFactory::setColourScheme(const String& name)
{
	bool found = false;

	for (auto& scheme: colourSchemes) {
		if (scheme.second == name) {
			setColourSchemeByAssetId(scheme.first);
			found = true;
			break;
		}
	}

	if (!found && !colourSchemes.empty()) {
		setColourSchemeByAssetId(colourSchemes.front().first);
		found = true;
	}

	if (found) {
		reloadStyleSheet();
	}
}

void EditorUIFactory::setProject(ProjectWindow* projectWindow, Resources* gameResources)
{
	this->projectWindow = projectWindow;
	this->gameResources = gameResources;
}

void EditorUIFactory::setColourSchemeByAssetId(const String& assetId)
{
	colourScheme = std::make_shared<UIColourScheme>(getResources().get<ConfigFile>(assetId)->getRoot(), getResources());
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
