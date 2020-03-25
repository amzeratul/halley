#include "assets_editor_window.h"
#include "halley/tools/project/project.h"
#include "halley/core/resources/resource_locator.h"
#include "halley/core/resources/standard_resources.h"
#include "halley/ui/widgets/ui_label.h"
#include "halley/ui/widgets/ui_list.h"
#include "animation_editor.h"
#include "metadata_editor.h"

using namespace Halley;

AssetsEditorWindow::AssetsEditorWindow(UIFactory& factory, Project& project)
	: UIWidget("assets_editor", {}, UISizer())
	, factory(factory)
	, project(project)
	, curSrcPath(".")
{
	loadResources();
	makeUI();
	setAssetSrcMode(true);
}

void AssetsEditorWindow::loadResources()
{
	project.addAssetReloadCallback([=] (const std::vector<String>& assets)
	{
		refreshAssets(assets);
	});
}

void AssetsEditorWindow::makeUI()
{
	UIWidget::add(factory.makeUI("ui/halley/assets_editor_window"), 1);

	assetList = getWidgetAs<UIList>("assetList");
	assetList->setSingleClickAccept(false);
	metadataEditor = getWidgetAs<MetadataEditor>("metadataEditor");
	content = getWidgetAs<UIPagedPane>("content");
	contentList = getWidgetAs<UIList>("contentList");

	setHandle(UIEventType::ListSelectionChanged, "contentList", [=] (const UIEvent& event)
	{
		content->setPage(event.getData().toInteger());
	});

	setHandle(UIEventType::ListSelectionChanged, "assetType", [=] (const UIEvent& event)
	{
		listAssets(fromString<AssetType>(event.getData()));
	});

	setHandle(UIEventType::ListSelectionChanged, "assetList", [=] (const UIEvent& event)
	{
		loadAsset(event.getData(), false);
	});

	setHandle(UIEventType::ListAccept, "assetList", [=] (const UIEvent& event)
	{
		loadAsset(event.getData(), true);
	});

	setHandle(UIEventType::TextChanged, "assetSearch", [=] (const UIEvent& event)
	{
		setFilter(event.getData());
	});
}

void AssetsEditorWindow::setAssetSrcMode(bool enabled)
{
	assetSrcMode = enabled;
	getWidget("assetType")->setActive(!assetSrcMode);
	if (assetSrcMode) {
		listAssetSources();
	} else {
		listAssets(AssetType::Sprite);
	}
}

void AssetsEditorWindow::listAssetSources()
{
	if (!assetNames) {
		assetNames = project.getAssetSrcList();
		std::sort(assetNames->begin(), assetNames->end()); // Is this even needed?
	}

	if (filter.isEmpty()) {
		setListContents(assetNames.value(), curSrcPath, false);
	} else {
		std::vector<String> filteredList;
		for (auto& a: assetNames.value()) {
			if (a.asciiLower().contains(filter)) {
				filteredList.push_back(a);
			}
		}
		setListContents(filteredList, curSrcPath, true);
	}	
}

void AssetsEditorWindow::listAssets(AssetType type)
{
	curType = type;
	if (curPaths.find(type) == curPaths.end()) {
		curPaths[type] = Path(".");
	}
	const auto curPath = curPaths[type];

	auto assets = project.getGameResources().ofType(type).enumerate();
	std::sort(assets.begin(), assets.end());

	setListContents(assets, curPath, false);
}

void AssetsEditorWindow::setListContents(std::vector<String> assets, const Path& curPath, bool flat)
{
	assetList->clear();
	if (flat) {
		for (auto& a: assets) {
			assetList->addTextItem(a, LocalisedString::fromUserString(Path(a).getFilename().toString()));
		}
	} else {
		std::set<String> dirs;
		std::vector<std::pair<String, String>> files;

		for (auto& a: assets) {
			auto relPath = Path("./" + a).makeRelativeTo(curPath);
			if (relPath.getNumberPaths() == 1) {
				files.emplace_back(a, relPath.toString());
			} else {
				auto start = relPath.getFront(1);
				dirs.insert(start.toString());
			}
		}

		for (auto& dir: dirs) {
			assetList->addTextItem(dir + "/.", LocalisedString::fromUserString("[" + dir + "]"));
		}
		for (auto& file: files) {
			assetList->addTextItem(file.first, LocalisedString::fromUserString(file.second));
		}
	}
}

void AssetsEditorWindow::refreshList()
{
	if (assetSrcMode) {
		listAssetSources();
	} else {
		listAssets(curType);
	}
}

void AssetsEditorWindow::setFilter(const String& f)
{
	if (filter != f) {
		filter = f.asciiLower();
		refreshList();
	}
}

void AssetsEditorWindow::loadAsset(const String& name, bool doubleClick)
{
	auto& curPath = assetSrcMode ? curSrcPath : curPaths[curType];
	if (name.endsWith("/.")) {
		if (doubleClick) {
			curPath = curPath / name;
			refreshList();
		}
	} else {
		content->clear();
		contentList->clear();
		curEditors.clear();

		if (assetSrcMode) {
			auto assets = project.getAssetsFromFile(Path(name));
			std::sort(assets.begin(), assets.end(), [] (decltype(assets)::const_reference a, decltype(assets)::const_reference b) -> bool
			{
				return b.first < a.first;
			});
			for (auto& asset: assets) {
				createEditorTab(asset.first, asset.second);
			}

			if (assets.empty()) {
				metadataEditor->clear();
			} else {
				const auto type = assets.at(0).first;
				auto effectiveMeta = project.getImportMetadata(type, assets.at(0).second);
				metadataEditor->setResource(project, type, Path(name), std::move(effectiveMeta));
			}
		} else {
			metadataEditor->clear();
			createEditorTab(curType, name);
		}
	}
}

void AssetsEditorWindow::refreshAssets(const std::vector<String>& assets)
{
	assetNames.reset();
	refreshList();

	for (auto& editor: curEditors) {
		editor->reload();
	}
}

std::shared_ptr<AssetEditor> AssetsEditorWindow::makeEditor(AssetType type, const String& name)
{
	switch (type) {
	case AssetType::Sprite:
	case AssetType::Animation:
	case AssetType::Texture:
		return std::make_shared<AnimationEditor>(factory, project.getGameResources(), type, project);
	}
	return {};
}

void AssetsEditorWindow::createEditorTab(AssetType type, const String& name)
{
	auto editor = makeEditor(type, name);
	if (editor) {
		editor->setResource(name);
		int n = content->getNumberOfPages();
		content->addPage();
		content->getPage(n)->add(editor, 1);
		contentList->addTextItem(toString(n), LocalisedString::fromUserString(toString(type)));
	}
}

AssetEditor::AssetEditor(UIFactory& factory, Resources& resources, Project& project, AssetType type)
	: UIWidget("assetEditor", {}, UISizer())
	, factory(factory)
	, project(project)
	, resources(resources)
	, assetType(type)
{
}

void AssetEditor::setResource(const String& id)
{
	assetId = id;
	if (assetType == AssetType::Animation) {
		resource = resources.get<Animation>(assetId);
	} else if (assetType == AssetType::Sprite) {
		resource = resources.get<SpriteResource>(assetId);
	} else if (assetType == AssetType::Texture) {
		resource = resources.get<Texture>(assetId);
	}
	reload();
}

void AssetEditor::clearResource()
{
	assetId = "";
	resource.reset();
	reload();
}

void AssetEditor::reload()
{
}
