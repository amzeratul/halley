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
	auto list = getWidgetAs<UIList>("assetList");
	list->clear();
	auto& res = *gameResources;

	auto assets = res.ofType(type).enumerate();
	std::sort(assets.begin(), assets.end());
	for (auto& a: assets) {
		list->addTextItem(a, LocalisedString::fromUserString(a));
	}
}
