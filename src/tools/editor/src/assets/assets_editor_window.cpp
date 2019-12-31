#include "assets_editor_window.h"
#include "halley/tools/project/project.h"
#include "halley/core/resources/resource_locator.h"
#include "halley/core/resources/standard_resources.h"
#include "halley/ui/widgets/ui_label.h"
#include "halley/ui/widgets/ui_list.h"
#include "animation_editor.h"

using namespace Halley;

AssetsEditorWindow::AssetsEditorWindow(UIFactory& factory, Project& project, const HalleyAPI& api)
	: UIWidget("assets_editor", {}, UISizer())
	, factory(factory)
	, project(project)
{
	loadResources(api);
	makeUI();
	listAssets(AssetType::Sprite);
}

void AssetsEditorWindow::makeUI()
{
	UIWidget::add(factory.makeUI("ui/halley/assets_editor_window"), 1);
	getWidgetAs<UIList>("assetList")->setSingleClickAccept(false);

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
	try {
		locator->addFileSystem(project.getUnpackedAssetsPath());
	} catch (...)
	{}

	gameResources = std::make_unique<Resources>(std::move(locator), api);
	StandardResources::initialize(*gameResources);

	project.addAssetReloadCallback([=] (const std::vector<String>& assets)
	{
		refreshAssets(assets);
	});
}

void AssetsEditorWindow::refreshAssets(const std::vector<String>& assets)
{
	if (gameResources->getLocator().getLocatorCount() == 0) {
		try {
			gameResources->getLocator().addFileSystem(project.getUnpackedAssetsPath());
		} catch (...)
		{}
	}

	gameResources->reloadAssets(assets);
	listAssets(curType);
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
	auto& curPath = curPaths[curType];
	if (name.endsWith("/.")) {
		curPath = curPath / name;
		listAssets(curType);
	} else {
		getWidget("contents")->clear();

		const auto assetName = (curPath / name).toString().mid(2);
		const auto editor = createEditor(curType, assetName);
		if (editor) {
			getWidget("contents")->add(editor, 1);
			getWidgetAs<UILabel>("assetName")->setText(LocalisedString::fromUserString("[" + toString(curType) + "] " + assetName));
		}
	}
}

std::shared_ptr<UIWidget> AssetsEditorWindow::createEditor(AssetType type, const String& name)
{
	switch (type) {
	case AssetType::Sprite:
	case AssetType::Animation:
	case AssetType::Texture:
		return std::make_shared<AnimationEditor>(factory, *gameResources, project, name, type);
	}
	return {};
}
