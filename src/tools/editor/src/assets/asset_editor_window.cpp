#include "asset_editor_window.h"
#include "animation_editor.h"
#include "asset_editor.h"
#include "metadata_editor.h"
#include "prefab_editor.h"
#include "graph/render_graph_editor.h"
#include "halley/tools/project/project.h"
#include "src/ui/editor_ui_factory.h"
#include "src/ui/project_window.h"
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
	metadataEditor = getWidgetAs<MetadataEditor>("metadataEditor");
	
	setHandle(UIEventType::ListSelectionChanged, "contentList", [=] (const UIEvent& event)
	{
		content->setPage(event.getStringData().toInteger());
	});

	setHandle(UIEventType::ButtonClicked, "openFile", [=] (const UIEvent& event)
	{
		projectWindow.openFileExternally(getCurrentAssetPath());
	});

	setHandle(UIEventType::ButtonClicked, "showFile", [=] (const UIEvent& event)
	{
		projectWindow.showFileExternally(getCurrentAssetPath());
	});
}

void AssetEditorWindow::setAssetSrcMode(bool assetSrcMode)
{
	this->assetSrcMode = assetSrcMode;
}

void AssetEditorWindow::loadAsset(const String& name, std::optional<AssetType> type, bool force)
{
	bool showMetadataEditor = true;

	if (loadedAsset != name || force) {
		loadedAsset = name;
		loadedType = type;
		
		content->clear();
		contentList->clear();
		curEditors.clear();

		if (assetSrcMode) {
			auto assets = project.getAssetsFromFile(Path(name));
			lastAssets = assets;

			std::sort(assets.begin(), assets.end(), [] (decltype(assets)::const_reference a, decltype(assets)::const_reference b) -> bool
			{
				return b.first < a.first;
			});

			for (const auto& asset: assets) {
				if (asset.first == AssetType::Prefab || asset.first == AssetType::Scene) {
					showMetadataEditor = false;
				}
			}

			for (auto& asset: assets) {
				createEditorTab(Path(name), asset.first, asset.second);
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

bool AssetEditorWindow::isModified() const
{
	if (metadataEditor->isModified()) {
		return true;
	}
	for (const auto& editor: curEditors) {
		if (editor->isModified()) {
			return true;
		}
	}
	return false;
}

void AssetEditorWindow::save()
{
	if (metadataEditor->isModified()) {
		metadataEditor->saveMetadata();
	}
	for (const auto& editor: curEditors) {
		if (editor->isModified()) {
			editor->save();
		}
	}
}

String AssetEditorWindow::getName() const
{
	return Path(loadedAsset).getFilename().getString();
}

void AssetEditorWindow::onDoubleClickAsset()
{
	if (!curEditors.empty()) {
		curEditors.front()->onDoubleClick();
	}
}

void AssetEditorWindow::refreshAssets()
{
	auto assets = project.getAssetsFromFile(Path(loadedAsset));
	if (assets != lastAssets) {
		loadAsset(loadedAsset, loadedType, true);
	} else {
		for (auto& editor: curEditors) {
			editor->refreshAssets();
		}
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
		const auto text = std::make_shared<UILabel>(name + "_" + toString(type) + ":label", contentList->getStyle(), LocalisedString::fromUserString(name));
		
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
	case AssetType::RenderGraphDefinition:
		return std::make_shared<RenderGraphEditor>(factory, project.getGameResources(), project, type);
	}
	return {};
}
