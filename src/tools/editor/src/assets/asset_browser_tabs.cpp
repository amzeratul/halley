#include "asset_browser_tabs.h"

#include "asset_editor_window.h"
#include "src/ui/editor_ui_factory.h"

using namespace Halley;

AssetBrowserTabs::AssetBrowserTabs(UIFactory& factory, Project& project, ProjectWindow& projectWindow)
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

	// Not found, add it instead
	auto tabContents = factory.makeUI("ui/halley/asset_browser_tab_contents");
	tabContents->getWidgetAs<UILabel>("label")->setText(LocalisedString::fromHardcodedString(Path(name).getFilename().toString()));
	tabContents->setHandle(UIEventType::ButtonClicked, "close", [=] (const UIEvent& event)
	{
		toClose.push_back(key);
	});
	tabs->addItem(key, tabContents);

	auto window = std::make_shared<AssetEditorWindow>(dynamic_cast<EditorUIFactory&>(factory), project, projectWindow);
	window->setAssetSrcMode(srcMode);
	window->loadAsset(name, assetType, true);
	
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
	}
}
