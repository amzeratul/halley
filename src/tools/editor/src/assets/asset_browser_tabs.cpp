#include "asset_browser_tabs.h"

#include "asset_editor_window.h"
#include "halley/tools/project/project.h"
#include "src/ui/editor_ui_factory.h"

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
	const String key = assetType ? (toString(assetType) + ":" + name) : name;

	const bool alreadyExists = tabs->setSelectedOptionId(key);
	if (alreadyExists) {
		return;
	}

	// Create tab
	Sprite icon;
	if (assetType) {
		icon = factory.makeAssetTypeIcon(assetType.value());
	} else {
		const auto type = project.getAssetImporter()->getImportAssetType(name, false);
		icon = factory.makeImportAssetTypeIcon(type);
	}
	auto label = LocalisedString::fromUserString(Path(name).getFilename().toString());
	auto tabContents = factory.makeUI("ui/halley/asset_browser_tab_contents");
	tabContents->getWidgetAs<UIImage>("icon")->setSprite(icon);
	tabContents->getWidgetAs<UILabel>("label")->setText(std::move(label));
	tabContents->setHandle(UIEventType::ButtonClicked, "close", [=] (const UIEvent& event)
	{
		toClose.push_back(key);
	});
	tabs->addItem(key, tabContents);

	// Create window
	auto window = std::make_shared<AssetEditorWindow>(factory, project, projectWindow);
	window->setAssetSrcMode(srcMode);
	window->loadAsset(name, assetType, true);
	windows.push_back(window);

	// Add to tabs
	pages->addPage()->add(window, 1);
	tabs->setSelectedOption(int(tabs->getCount()) - 1);
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
	});
}

void AssetBrowserTabs::closeTab(const String& key)
{
	auto idx = tabs->removeItem(key);
	if (idx) {
		pages->removePage(static_cast<int>(idx.value()));
		windows.erase(windows.begin() + idx.value());
	}
}
