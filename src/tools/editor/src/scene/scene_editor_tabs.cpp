#include "scene_editor_tabs.h"

#include "scene_editor_window.h"
using namespace Halley;

SceneEditorTabs::SceneEditorTabs(UIFactory& factory, Project& project, const HalleyAPI& api)
	: UIWidget("scene_editor_tabs", {}, UISizer(UISizerType::Horizontal))
	, factory(factory)
	, project(project)
	, api(api)
{
	makeUI();
}

void SceneEditorTabs::load(AssetType assetType, const String& name)
{
	const String key = toString(assetType) + ":" + name;

	const auto n = tabs->getCount();
	for (int i = 0; i < n; ++i) {
		if (tabs->getItem(i)->getId() == key) {
			tabs->setSelectedOption(i);
			return;
		}
	}

	// Not found, add it instead
	tabs->addTextItem(key, LocalisedString::fromHardcodedString(Path(name).getFilename().toString()));
	auto window = std::make_shared<SceneEditorWindow>(factory, project, api);
	if (assetType == AssetType::Scene) {
		window->loadScene(name);
	} else if (assetType == AssetType::Prefab) {
		window->loadPrefab(name);
	}
	pages->addPage()->add(window, 1);
	tabs->setSelectedOption(tabs->getCount() - 1);
}

void SceneEditorTabs::makeUI()
{
	add(factory.makeUI("ui/halley/scene_editor_tabs"), 1);
	tabs = getWidgetAs<UIList>("tabs");
	pages = getWidgetAs<UIPagedPane>("pages");

	setHandle(UIEventType::ListSelectionChanged, "tabs", [=] (const UIEvent& event)
	{
		pages->setPage(event.getIntData());
	});
}
