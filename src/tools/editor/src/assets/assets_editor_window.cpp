#include "assets_editor_window.h"
#include "halley/tools/project/project.h"
#include "halley/core/resources/standard_resources.h"
using namespace Halley;

AssetsEditorWindow::AssetsEditorWindow(UIFactory& factory, Project& project, const HalleyAPI& api)
	: UIWidget("assets_editor", {}, UISizer())
	, factory(factory)
	, project(project)
{
	loadResources(api);
	makeUI();
	listAssets(AssetType::Animation);
}

void AssetsEditorWindow::makeUI()
{
	UIWidget::add(factory.makeUI("ui/halley/assets_editor_window"), 1);

	setHandle(UIEventType::ListSelectionChanged, "assetType", [=] (const UIEvent& event)
	{
		listAssets(fromString<AssetType>(event.getData()));
	});

	setHandle(UIEventType::ListAccept, "assetList", [=] (const UIEvent& event)
	{
		loadAsset(event.getData());
	});
}

void AssetsEditorWindow::loadResources(const HalleyAPI& api)
{
	auto locator = std::make_unique<ResourceLocator>(*api.system);
	locator->addFileSystem(project.getUnpackedAssetsPath());
	gameResources = std::make_unique<Resources>(std::move(locator), api);
	StandardResources::initialize(*gameResources);
}

void AssetsEditorWindow::listAssets(AssetType type)
{
	curType = type;
	if (curPaths.find(type) == curPaths.end()) {
		curPaths[type] = Path(".");
	}
	const auto curPath = curPaths[type];
	
	auto assets = gameResources->ofType(type).enumerate();
	std::sort(assets.begin(), assets.end());

	std::set<String> dirs;
	std::vector<String> files;

	for (auto& a: assets) {
		auto path = Path("./" + a);
		auto relPath = path.makeRelativeTo(curPath);
		if (relPath.getNumberPaths() == 1) {
			files.push_back(relPath.toString());
		} else {
			auto start = relPath.getFront(1);
			dirs.insert(start.toString());
		}
	}

	auto list = getWidgetAs<UIList>("assetList");
	list->clear();
	for (auto& dir: dirs) {
		list->addTextItem(dir + "/.", LocalisedString::fromUserString("[" + dir + "]"));
	}
	for (auto& file: files) {
		list->addTextItem(file, LocalisedString::fromUserString(file));
	}
}

void AssetsEditorWindow::loadAsset(const String& name)
{
	if (name.endsWith("/.")) {
		auto& curPath = curPaths[curType];
		curPath = curPath / name;
		listAssets(curType);
	} else {
		// TODO
	}
}
