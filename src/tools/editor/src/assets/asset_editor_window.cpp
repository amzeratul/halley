#include "asset_editor_window.h"
#include "animation_editor.h"
#include "asset_editor.h"
#include "metadata_editor.h"
#include "prefab_editor.h"
#include "halley/tools/project/project.h"
#include "src/ui/editor_ui_factory.h"
using namespace Halley;

AssetEditorWindow::AssetEditorWindow(EditorUIFactory& factory, Project& project, ProjectWindow& projectWindow)
	: UIWidget("assetEditorWindow", Vector2f(), UISizer())
	, factory(factory)
	, project(project)
	, projectWindow(projectWindow)
{
	factory.loadUI(*this, "ui/halley/asset_editor_window");
}

void AssetEditorWindow::onMakeUI()
{
	content = getWidgetAs<UIPagedPane>("content");
	contentList = getWidgetAs<UIList>("contentList");
	contentListDropdown = getWidgetAs<UIDropdown>("contentListDropdown");
	metadataEditor = getWidgetAs<MetadataEditor>("metadataEditor");
	
	getWidget("contentListDropdownArea")->setActive(false);
	
	setHandle(UIEventType::ListSelectionChanged, "contentList", [=] (const UIEvent& event)
	{
		content->setPage(event.getStringData().toInteger());
	});

	setHandle(UIEventType::DropboxSelectionChanged, "contentListDropdown", [=](const UIEvent& event)
	{
		loadAsset(loadedAsset, loadedType, false, true);
	});

	setHandle(UIEventType::ButtonClicked, "openFile", [=] (const UIEvent& event)
	{
		openFileExternally(getCurrentAssetPath());
	});

	setHandle(UIEventType::ButtonClicked, "showFile", [=] (const UIEvent& event)
	{
		showFileExternally(getCurrentAssetPath());
	});
}

void AssetEditorWindow::setAssetSrcMode(bool assetSrcMode)
{
	this->assetSrcMode = assetSrcMode;
}

void AssetEditorWindow::loadAsset(const String& name, std::optional<AssetType> type, bool clearDropdown, bool force)
{
	if (clearDropdown && loadedAsset != name) {
		contentListDropdown->clear();
		getWidget("contentListDropdownArea")->setActive(false);
	}

	bool showMetadataEditor = true;

	if (loadedAsset != name || force) {
		loadedAsset = name;
		loadedType = type;
		
		content->clear();
		contentList->clear();
		curEditors.clear();

		if (assetSrcMode) {
			auto assets = project.getAssetsFromFile(Path(name));

			std::sort(assets.begin(), assets.end(), [] (decltype(assets)::const_reference a, decltype(assets)::const_reference b) -> bool
			{
				return b.first < a.first;
			});

			for (const auto& asset: assets) {
				if (asset.first == AssetType::Prefab || asset.first == AssetType::Scene) {
					showMetadataEditor = false;
				}
			}

			auto useDropdown = false;
			std::vector<String> dropdownAssetIds;
			if (assets.size() > 1) {
				for (const auto& asset : assets) {
					if (asset.first == AssetType::Animation) {
						dropdownAssetIds.push_back(asset.second);
					}
				}

				useDropdown = dropdownAssetIds.size() > 1;

				if (useDropdown && clearDropdown) {
					contentListDropdown->setOptions(dropdownAssetIds, 0);
				}
			}
			getWidget("contentListDropdownArea")->setActive(useDropdown);
			
			for (auto& asset: assets) {
				if (!useDropdown || asset.second == contentListDropdown->getSelectedOptionId() || std::find(dropdownAssetIds.begin(), dropdownAssetIds.end(), asset.second) == dropdownAssetIds.end()) {
					createEditorTab(Path(name), asset.first, asset.second);
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
			createEditorTab(Path(name), type.value(), name);
		}
	}

	getWidget("metadataPanel")->setActive(showMetadataEditor);
}

Path AssetEditorWindow::getCurrentAssetPath() const
{
	return project.getAssetsSrcPath() / loadedAsset;
}

void AssetEditorWindow::onDoubleClickAsset()
{
	if (!curEditors.empty()) {
		curEditors.front()->onDoubleClick();
	}
}

void AssetEditorWindow::refreshAssets()
{
	for (auto& editor: curEditors) {
		editor->reload();
	}
}

void AssetEditorWindow::createEditorTab(Path filePath, AssetType type, const String& name)
{
	auto editor = makeEditor(std::move(filePath), type, name);
	if (editor) {
		editor->setResource(name);
		auto n = content->getNumberOfPages();
		content->addPage();
		content->getPage(n)->add(editor, 1);
		const auto image = std::make_shared<UIImage>(factory.makeAssetTypeIcon(type));
		const auto text = std::make_shared<UILabel>(name + "_" + toString(type) + ":label", contentList->getStyle().getTextRenderer("label"), LocalisedString::fromUserString(name));
		
		auto item = std::make_shared<UISizer>();
		item->add(image);
		item->add(text, 1.0f, {}, UISizerAlignFlags::CentreVertical);

		contentList->addItem(toString(n), item);
		contentList->setActive(contentList->getCount() > 1);
		curEditors.push_back(editor);
	}
}

std::shared_ptr<AssetEditor> AssetEditorWindow::makeEditor(Path filePath, AssetType type, const String& name)
{
	switch (type) {
	case AssetType::Sprite:
		return {};
	case AssetType::Animation:
	case AssetType::Texture:
		return std::make_shared<AnimationEditor>(factory, project.getGameResources(), type, project, *metadataEditor);
	case AssetType::Prefab:
	case AssetType::Scene:
		return std::make_shared<PrefabEditor>(factory, project.getGameResources(), type, project, projectWindow);
	}
	return {};
}

void AssetEditorWindow::openFileExternally(const Path& path)
{
	auto cmd = "start \"\" \"" + path.toString().replaceAll("/", "\\") + "\"";
	system(cmd.c_str());
}

void AssetEditorWindow::showFileExternally(const Path& path)
{
	auto cmd = "explorer.exe /select,\"" + path.toString().replaceAll("/", "\\") + "\"";
	system(cmd.c_str());
}
