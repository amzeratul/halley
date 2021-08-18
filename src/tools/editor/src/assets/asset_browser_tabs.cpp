#include "asset_browser_tabs.h"

#include "asset_editor_window.h"
#include "halley/tools/project/project.h"
#include "src/ui/editor_ui_factory.h"
#include "src/ui/project_window.h"

using namespace Halley;

AssetBrowserTabs::AssetBrowserTabs(EditorUIFactory& factory, Project& project, ProjectWindow& projectWindow)
	: UIWidget("asset_browser_tabs", {}, UISizer(UISizerType::Horizontal))
	, factory(factory)
	, project(project)
	, projectWindow(projectWindow)
{
	makeUI();
}

void AssetBrowserTabs::load(std::optional<AssetType> assetType, const String& name)
{
	openTab(assetType, name, true);
	saveTabs();
}

void AssetBrowserTabs::openTab(std::optional<AssetType> assetType, const String& name, bool selected)
{
	const String key = assetType ? (toString(assetType) + ":" + name) : name;

	const bool alreadyExists = tabs->setSelectedOptionId(key);
	if (alreadyExists) {
		return;
	}

	// Create tab
	auto tabContents = factory.makeUI("ui/halley/asset_browser_tab_contents");
	tabContents->setId("tabContents");
	populateTab(*tabContents, assetType, name, key);
	tabs->addItem(key, tabContents);

	// Create window
	auto window = std::make_shared<AssetEditorWindow>(factory, project, projectWindow);
	window->setAssetSrcMode(srcMode);
	window->loadAsset(name, assetType, true);
	windows.push_back(window);

	// Add to tabs
	pages->addPage()->add(window, 1);
	if (selected) {
		tabs->setSelectedOption(int(tabs->getCount()) - 1);
	}
}

void AssetBrowserTabs::closeTab(const String& key)
{
	const auto idx = tabs->tryGetItemId(key);
	if (idx != -1) {
		pages->removePage(idx);
		windows.erase(windows.begin() + idx);
		tabs->removeItem(key);
		saveTabs();
	}
}

void AssetBrowserTabs::populateTab(UIWidget& tab, std::optional<AssetType> assetType, const String& name, const String& key)
{
	Sprite icon;
	if (assetType) {
		icon = factory.makeAssetTypeIcon(assetType.value());
	} else {
		const auto type = project.getAssetImporter()->getImportAssetType(name, false);
		icon = factory.makeImportAssetTypeIcon(type);
	}
	auto label = LocalisedString::fromUserString(Path(name).getFilename().toString());
	tab.getWidgetAs<UIImage>("icon")->setSprite(icon);
	tab.getWidgetAs<UILabel>("label")->setText(std::move(label));
	tab.setHandle(UIEventType::ButtonClicked, "close", [=] (const UIEvent& event)
	{
		toClose.push_back(key);
	});
}

void AssetBrowserTabs::replaceAssetTab(const String& oldName, const String& newName)
{
	const auto idx = tabs->tryGetItemId(oldName);
	if (idx == -1) {
		return;
	}

	const auto contents = tabs->getItem(idx)->getWidget("tabContents");
	if (contents) {
		tabs->changeItemId(idx, newName);
		populateTab(*contents, {}, newName, newName);
		windows[idx]->loadAsset(newName, {}, true);
	}
}

void AssetBrowserTabs::refreshAssets()
{
	for (int i = 0; i < pages->getNumberOfPages(); ++i) {
		auto assetEditor = pages->getPage(i)->getWidgetAs<AssetEditorWindow>("assetEditorWindow");
		assetEditor->refreshAssets();
	}
}

void AssetBrowserTabs::setAssetSrcMode(bool srcMode)
{
	this->srcMode = srcMode;

	if (waitingLoad) {
		waitingLoad = false;
		loadTabs();
	}
}

void AssetBrowserTabs::update(Time t, bool moved)
{
	for (auto& key: toClose) {
		closeTab(key);
	}
	toClose.clear();

	int size = static_cast<int>(windows.size());
	for (int i = 0; i < size; ++i) {
		tabs->getItem(i)->getWidget("modified")->setActive(windows[i]->isModified());
	}
}

void AssetBrowserTabs::makeUI()
{
	add(factory.makeUI("ui/halley/asset_browser_tabs"), 1);
	tabs = getWidgetAs<UIList>("tabs");
	pages = getWidgetAs<UIPagedPane>("pages");

	setHandle(UIEventType::ListSelectionChanged, "tabs", [=] (const UIEvent& event)
	{
		pages->setPage(event.getIntData());
		saveTabs();
	});
}

void AssetBrowserTabs::saveTabs()
{
	ConfigNode::SequenceType tabIds;
	const auto n = tabs->getCount();
	for (int i = 0; i < n; ++i) {
		tabIds.push_back(ConfigNode(tabs->getItem(i)->getId()));
	}

	projectWindow.setSetting(EditorSettingType::Project, "tabsOpen", std::move(tabIds));
	projectWindow.setSetting(EditorSettingType::Project, "currentTab", ConfigNode(tabs->getSelectedOptionId()));
}

void AssetBrowserTabs::loadTabs()
{
	const auto& tabsOpen = projectWindow.getSetting(EditorSettingType::Project, "tabsOpen");
	if (tabsOpen.getType() == ConfigNodeType::Sequence) {
		for (const auto& tab: tabsOpen) {
			auto id = tab.asString();
			auto splitParts = id.split(':');
			if (splitParts.size() == 2) {
				openTab(fromString<AssetType>(splitParts[0]), splitParts[1], false);
			} else if (splitParts.size() == 1) {
				openTab({}, splitParts[0], false);
			}
		}
	}

	const auto curTab = projectWindow.getSetting(EditorSettingType::Project, "currentTab").asString("");
	tabs->setSelectedOptionId(curTab);
}
