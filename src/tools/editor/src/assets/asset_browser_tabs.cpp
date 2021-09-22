#include "asset_browser_tabs.h"

#include "asset_editor_window.h"
#include "halley/tools/project/project.h"
#include "src/ui/confirmation_popup.h"
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
	window->loadAsset(name, assetType);
	windows.push_back(window);

	// Add to tabs
	pages->addPage()->add(window, 1);
	layout();
	if (selected) {
		tabs->setSelectedOption(int(tabs->getCount()) - 1);
	}
}

bool AssetBrowserTabs::closeTab(const String& key)
{
	tabs->setSelectedOptionId(key);
	auto idx = tabs->getSelectedOption();

	if (windows[idx]->isModified()) {
		if (getRoot() && !getRoot()->hasModalUI()) {
			auto buttons = { ConfirmationPopup::ButtonType::Yes, ConfirmationPopup::ButtonType::No, ConfirmationPopup::ButtonType::Cancel };
			auto callback = [this, idx, key] (ConfirmationPopup::ButtonType buttonType)
			{
				if (buttonType == ConfirmationPopup::ButtonType::Cancel) {
					std_ex::erase_if(toClose, [&] (const auto& v) { return v == key; });
				} else {
					if (buttonType == ConfirmationPopup::ButtonType::Yes) {
						windows[idx]->save();
					}
					doCloseTab(key);
				}
			};

			getRoot()->addChild(std::make_shared<ConfirmationPopup>(factory, "Save Changes?", "Would you like to save your changes to " + windows[idx]->getName() + " before closing the tab?", buttons, std::move(callback)));
		}

		std_ex::erase_if(toClose, [&] (const auto& v) { return v == key; });
		return false;
	} else {
		doCloseTab(key);
	}
	return true;
}

void AssetBrowserTabs::doCloseTab(const String& key)
{
	const auto idx = tabs->tryGetItemId(key);
	if (idx != -1) {
		pages->removePage(idx);
		windows.erase(windows.begin() + idx);
		tabs->removeItem(key);
		saveTabs();
		std_ex::erase_if(toClose, [&] (const auto& v) { return v == key; });
	}
}

bool AssetBrowserTabs::requestQuit(std::function<void()> callback)
{
	if (!getRoot()) {
		return true;
	}
	if (quittingCallback) {
		Logger::logDev("Ignoring quit request due to another quest being in flight");
		return false;
	}
	if (getRoot()->hasModalUI()) {
		Logger::logDev("Ignoring quit request due to a modal UI being present: " + getRoot()->getModalUIName());
		return false;
	}

	quittingCallback = std::move(callback);
	return proceedQuitRequested(0, false);
}

bool AssetBrowserTabs::proceedQuitRequested(size_t idx, bool invoke)
{
	for (size_t i = idx; i < windows.size(); ++i) {
		auto& window = windows[i];
		if (window->isModified()) {
			tabs->setSelectedOption(static_cast<int>(i));
			auto buttons = { ConfirmationPopup::ButtonType::Yes, ConfirmationPopup::ButtonType::No, ConfirmationPopup::ButtonType::Cancel };
			auto callback = [this, i] (ConfirmationPopup::ButtonType buttonType)
			{
				if (buttonType == ConfirmationPopup::ButtonType::Cancel) {
					quittingCallback = {};
				} else {
					if (buttonType == ConfirmationPopup::ButtonType::Yes) {
						windows[i]->save();
					}
					proceedQuitRequested(i + 1, true);
				}
			};
			getRoot()->addChild(std::make_shared<ConfirmationPopup>(factory, "Save Changes?", "Would you like to save your changes to " + window->getName() + " before closing the project?", buttons, std::move(callback)));
			return false;
		}
	}

	if (invoke) {
		quittingCallback();
	}
	quittingCallback = {};

	return true;
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
		windows[idx]->loadAsset(newName, {});
		saveTabs();
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

void AssetBrowserTabs::saveCurrentTab()
{
	const int curPage = pages->getCurrentPage();
	if (curPage >= 0 && curPage < static_cast<int>(windows.size())) {
		windows[curPage]->save();
	}
}

void AssetBrowserTabs::saveAllTabs()
{
	for (auto& window: windows) {
		window->save();
	}
}

void AssetBrowserTabs::closeCurrentTab()
{
	const int curPage = pages->getCurrentPage();
	if (curPage >= 0 && curPage < static_cast<int>(windows.size())) {
		closeTab(tabs->getItem(curPage)->getId());
	}
}

void AssetBrowserTabs::update(Time t, bool moved)
{
	auto closing = toClose;
	for (auto& close: closing) {
		closeTab(close);
	}

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
