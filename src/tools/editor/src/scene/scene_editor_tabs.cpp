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
	return;

	const String key = toString(assetType) + ":" + name;

	const bool alreadyExists = tabs->setSelectedOptionId(key);
	if (alreadyExists) {
		return;
	}

	// Not found, add it instead
	auto tabContents = factory.makeUI("ui/halley/scene_editor_tab_contents");
	tabContents->getWidgetAs<UILabel>("label")->setText(LocalisedString::fromHardcodedString(Path(name).getFilename().toString()));
	tabContents->setHandle(UIEventType::ButtonClicked, "close", [=] (const UIEvent& event)
	{
		toClose.push_back(key);
	});
	tabs->addItem(key, tabContents);

	/*
	auto window = std::make_shared<SceneEditorWindow>(factory, project, api, *this);
	if (assetType == AssetType::Scene) {
		window->loadScene(name);
	} else if (assetType == AssetType::Prefab) {
		window->loadPrefab(name);
	}
	pages->addPage()->add(window, 1);
	*/
	tabs->setSelectedOption(int(tabs->getCount()) - 1);
}

void SceneEditorTabs::update(Time t, bool moved)
{
	for (auto& key: toClose) {
		closeTab(key);
	}
	toClose.clear();
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

void SceneEditorTabs::closeTab(const String& key)
{
	auto idx = tabs->removeItem(key);
	if (idx) {
		pages->removePage(static_cast<int>(idx.value()));
	}
}
