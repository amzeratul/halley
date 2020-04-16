#include "editor_ui_factory.h"
#include "halley/core/resources/resources.h"
#include <halley/file_formats/config_file.h>
#include "scroll_background.h"
#include "src/assets/animation_editor.h"
#include "src/assets/metadata_editor.h"
#include "src/scene/entity_editor.h"
#include "src/scene/entity_list.h"
#include "src/scene/scene_editor_canvas.h"
using namespace Halley;

static std::shared_ptr<UIStyleSheet> makeStyleSheet(Resources& resources)
{
	auto result = std::make_shared<UIStyleSheet>(resources);
	for (auto& style: resources.enumerate<ConfigFile>()) {
		if (style.startsWith("ui_style/")) {
			result->load(*resources.get<ConfigFile>(style));
		}
	}
	return result;
}

EditorUIFactory::EditorUIFactory(const HalleyAPI& api, Resources& resources, I18N& i18n)
	: UIFactory(api, resources, i18n, makeStyleSheet(resources))
{
	UIInputButtons listButtons;
	setInputButtons("list", listButtons);

	addFactory("scrollBackground", [=] (const ConfigNode& node) { return makeScrollBackground(node); });
	addFactory("animationEditorDisplay", [=] (const ConfigNode& node) { return makeAnimationEditorDisplay(node); });
	addFactory("metadataEditor", [=] (const ConfigNode& node) { return makeMetadataEditor(node); });
	addFactory("sceneEditorCanvas", [=](const ConfigNode& node) { return makeSceneEditorCanvas(node); });
	addFactory("entityList", [=](const ConfigNode& node) { return makeEntityList(node); });
	addFactory("entityEditor", [=](const ConfigNode& node) { return makeEntityEditor(node); });
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
