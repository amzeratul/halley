#include "assets_editor_window.h"
#include "halley/tools/project/project.h"
#include "halley/core/resources/resource_locator.h"
#include "halley/core/resources/standard_resources.h"
#include "halley/ui/widgets/ui_label.h"
#include "halley/ui/widgets/ui_list.h"
#include "animation_editor.h"
#include "metadata_editor.h"
#include "new_asset_window.h"
#include "prefab_editor.h"
#include "halley/tools/file/filesystem.h"
#include "halley/tools/yaml/yaml_convert.h"

using namespace Halley;

AssetsEditorWindow::AssetsEditorWindow(UIFactory& factory, Project& project, EditorRootStage& stage)
	: UIWidget("assets_editor", {}, UISizer())
	, factory(factory)
	, project(project)
	, stage(stage)
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
	contentListDropdown = getWidgetAs<UIDropdown>("contentListDropdown");
	contentListDropdownLabel = getWidgetAs<UILabel>("contentListDropdownLabel");

	setHandle(UIEventType::ListSelectionChanged, "contentList", [=] (const UIEvent& event)
	{
		content->setPage(event.getStringData().toInteger());
	});

	setHandle(UIEventType::DropboxSelectionChanged, "contentListDropdown", [=](const UIEvent& event)
	{
		loadAsset(loadedAsset, false, false);
	});

	setHandle(UIEventType::ListSelectionChanged, "assetType", [=] (const UIEvent& event)
	{
		listAssets(fromString<AssetType>(event.getStringData()));
	});

	setHandle(UIEventType::ListSelectionChanged, "assetList", [=] (const UIEvent& event)
	{
		loadAsset(event.getStringData(), false, true);
	});

	setHandle(UIEventType::ListAccept, "assetList", [=] (const UIEvent& event)
	{
		loadAsset(event.getStringData(), true, true);
	});

	setHandle(UIEventType::TextChanged, "assetSearch", [=] (const UIEvent& event)
	{
		setFilter(event.getStringData());
	});

	setHandle(UIEventType::ButtonClicked, "addAsset", [=] (const UIEvent& event)
	{
		addAsset();
	});

	setHandle(UIEventType::ButtonClicked, "removeAsset", [=] (const UIEvent& event)
	{
		removeAsset();
	});

	updateAddRemoveButtons();
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

void AssetsEditorWindow::loadAsset(const String& name, bool doubleClick, bool clearDropdown)
{
	if (clearDropdown) {
		contentListDropdown->clear();
		contentListDropdown->setActive(false);
		contentListDropdownLabel->setActive(false);
	}

	lastClickedAsset = name;
	updateAddRemoveButtons();
	
	auto& curPath = assetSrcMode ? curSrcPath : curPaths[curType];
	if (name.endsWith("/.")) {
		if (doubleClick) {
			curPath = curPath / name;
			refreshList();
		}
	} else {
		if (loadedAsset != name || !clearDropdown) {
			loadedAsset = name;
			
			content->clear();
			contentList->clear();
			curEditors.clear();

			if (assetSrcMode) {
				auto assets = project.getAssetsFromFile(Path(name));

				std::sort(assets.begin(), assets.end(), [] (decltype(assets)::const_reference a, decltype(assets)::const_reference b) -> bool
				{
					return b.first < a.first;
				});

				auto useDropdown = false;
				std::vector<String> assetNames;
				if (int(assets.size()) > 3) {				
					for (const auto& asset : assets) {
						if (asset.first == AssetType::Animation) {
							assetNames.push_back(asset.second);
						}
					}

					if(!assetNames.empty())	{
						useDropdown = true;
						contentListDropdown->setActive(true);
						contentListDropdownLabel->setActive(true);
					}

					if (clearDropdown) {
						contentListDropdown->setOptions(assetNames, 0);
					}
				}
				
				for (auto& asset: assets) {
					if (!useDropdown || asset.second == contentListDropdown->getSelectedOptionId() || std::find(assetNames.begin(), assetNames.end(), asset.second) == assetNames.end()) {
						createEditorTab(asset.first, asset.second);
					}
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

		if (doubleClick) {
			onDoubleClickAsset();
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

void AssetsEditorWindow::onDoubleClickAsset()
{
	if (!curEditors.empty()) {
		curEditors.front()->onDoubleClick();
	}
}

std::shared_ptr<AssetEditor> AssetsEditorWindow::makeEditor(AssetType type, const String& name)
{
	switch (type) {
	case AssetType::Sprite:
	case AssetType::Animation:
	case AssetType::Texture:
		return std::make_shared<AnimationEditor>(factory, project.getGameResources(), type, project);
	case AssetType::Prefab:
	case AssetType::Scene:
		return std::make_shared<PrefabEditor>(factory, project.getGameResources(), type, project, stage);
	}
	return {};
}

void AssetsEditorWindow::createEditorTab(AssetType type, const String& name)
{
	auto editor = makeEditor(type, name);
	if (editor) {
		editor->setResource(name);
		auto n = content->getNumberOfPages();
		content->addPage();
		content->getPage(n)->add(editor, 1);
		auto typeSprite = Sprite().setImage(factory.getResources(), Path("ui") / "assetTypes" / toString(type) + ".png");
		const auto image = std::make_shared<UIImage>(typeSprite);
		const auto text = std::make_shared<UILabel>(name + "_" + toString(type) + ":label", contentList->getStyle().getTextRenderer("label"), LocalisedString::fromUserString(name));
		
		auto item = std::make_shared<UISizer>();
		item->add(image);
		item->add(text, 1.0f, {}, UISizerAlignFlags::CentreVertical);

		contentList->addItem(toString(n), item);
		curEditors.push_back(editor);
	}
}

void AssetsEditorWindow::updateAddRemoveButtons()
{
	// TODO: refactor updateAddRemoveButtons/addAsset/removeAsset?
	
	const auto stem = curSrcPath.getFront(1).string();
	const bool canAdd = stem == "prefab" || stem == "scene";
	const bool canRemove = !lastClickedAsset.isEmpty() && !lastClickedAsset.endsWith("/.");

	getWidget("addAsset")->setEnabled(canAdd);
	getWidget("removeAsset")->setEnabled(canRemove);
}

void AssetsEditorWindow::addAsset()
{
	// TODO: refactor updateAddRemoveButtons/addAsset/removeAsset?
	
	const auto assetType = curSrcPath.getFront(1).string();
	const auto dstPath = project.getAssetsSrcPath() / curSrcPath;

	getRoot()->addChild(std::make_shared<NewAssetWindow>(factory, [=] (std::optional<String> newName)
	{
		if (newName) {
			if (assetType == "prefab") {
				Prefab prefab;
				prefab.makeDefault();
				FileSystem::writeFile(dstPath / (newName.value() + ".prefab"), YAMLConvert::generateYAML(prefab.getRoot(), YAMLConvert::EmitOptions()));
			} else if (assetType == "scene") {
				Scene scene;
				scene.makeDefault();
				FileSystem::writeFile(dstPath / (newName.value() + ".scene"), YAMLConvert::generateYAML(scene.getRoot(), YAMLConvert::EmitOptions()));
			}
		}
	}));
}

void AssetsEditorWindow::removeAsset()
{
	// TODO: refactor updateAddRemoveButtons/addAsset/removeAsset?
	assetList->setItemActive(lastClickedAsset, false);
	FileSystem::remove(project.getAssetsSrcPath() / lastClickedAsset);
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

void AssetEditor::onDoubleClick()
{
}
