#include "asset_editor_window.h"
#include "animation_editor.h"
#include "asset_editor.h"
#include "asset_file_handler.h"
#include "font_editor.h"
#include "audio_editor/audio_event_editor.h"
#include "audio_editor/audio_object_editor.h"
#include "metadata_editor.h"
#include "prefab_editor.h"
#include "graph/render_graph_editor.h"
#include "halley/tools/project/project.h"
#include "src/ui/editor_ui_factory.h"
#include "src/ui/project_window.h"
#include "ui_editor/ui_editor.h"
using namespace Halley;

AssetEditorWindow::AssetEditorWindow(EditorUIFactory& factory, Project& project, ProjectWindow& projectWindow)
	: UIWidget("assetEditorWindow", Vector2f(), UISizer())
	, factory(factory)
	, project(project)
	, projectWindow(projectWindow)
{
	factory.loadUI(*this, "halley/asset_editor_window");
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

void AssetEditorWindow::reload()
{
	lastAssets = {};
	refreshAssets();
}

void AssetEditorWindow::loadAsset(const String& name, std::optional<AssetType> type, bool force)
{
	if (loadedAsset == name && !force) {
		return;
	}

	loadedAsset = name;
	loadedType = type;
		
	content->clear();
	contentList->clear();
	curEditors.clear();

	auto assets = getAssetsFromFile(Path(name));
	lastAssets = assets;

	std::sort(assets.begin(), assets.end(), [] (decltype(assets)::const_reference a, decltype(assets)::const_reference b) -> bool
	{
		return b.first < a.first;
	});

	bool showMetadataEditor = false;
	for (const auto& asset: assets) {
		if (MetadataEditor::hasEditorForType(asset.first)) {
			showMetadataEditor = true;
		}
	}

	if (!showMetadataEditor || assets.empty()) {
		metadataEditor->clear();
	} else {
		// TODO: this is all a bit dodgy tbh, metafile should be derived here probably?
		const auto type = assets.at(0).first;
		const auto primaryFilePath = project.getImportAssetsDatabase().getPrimaryInputFile(type, assets.at(0).second, true);
		if (primaryFilePath.isEmpty()) {
			metadataEditor->clear();
		} else {
			auto effectiveMeta = project.getImportAssetsDatabase().getMetadata(type, assets.at(0).second).value_or(Metadata());
			metadataEditor->setResource(project, type, primaryFilePath, std::move(effectiveMeta));
		}
	}

	const bool hasSpriteSheet = std_ex::contains_if(assets, [&] (const auto& a) { return a.first == AssetType::SpriteSheet; });
	for (auto& asset: assets) {
		if (asset.first == AssetType::Texture && hasSpriteSheet) {
			continue;
		}
		createEditorTab(Path(name), asset.first, asset.second);
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

bool AssetEditorWindow::canSave(bool forceInstantCheck) const
{
	for (const auto& editor: curEditors) {
		if (!editor->canSave(forceInstantCheck)) {
			return false;
		}
	}
	return true;
}

void AssetEditorWindow::save()
{
	if (metadataEditor->isModified()) {
		metadataEditor->saveMetadata();
	}
	for (const auto& editor: curEditors) {
		if (editor->canSave(true) && editor->isModified()) {
			editor->save();
		}
	}
}

String AssetEditorWindow::getName() const
{
	return Path(loadedAsset).getFilename().getString();
}

void AssetEditorWindow::onOpenAssetFinder(PaletteWindow& assetFinder)
{
	auto curPage = content->getCurrentPage();
	if (curPage >= 0 && curPage < static_cast<int>(curEditors.size())) {
		curEditors[curPage]->onOpenAssetFinder(assetFinder);
	}
}

void AssetEditorWindow::onDoubleClickAsset()
{
	if (!curEditors.empty()) {
		curEditors.front()->onDoubleClick();
	}
}

void AssetEditorWindow::refreshAssets()
{
	auto assets = getAssetsFromFile(Path(loadedAsset));
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
	if (const auto editor = makeEditor(type)) {
		editor->setResource(std::move(filePath), name);
		auto n = content->getNumberOfPages();
		content->addPage();
		content->getPage(n)->add(editor, 1);

		const auto shortName = Path(name).getFilename().string();
		const auto image = std::make_shared<UIImage>(factory.makeAssetTypeIcon(type));
		const auto text = std::make_shared<UILabel>(name + "_" + toString(type) + ":label", contentList->getStyle(), LocalisedString::fromUserString(shortName));
		
		auto item = std::make_shared<UISizer>();
		item->add(image);
		item->add(text, 1.0f, {}, UISizerAlignFlags::CentreVertical);

		auto listItem = contentList->addItem(toString(n), item);
		listItem->setToolTip(LocalisedString::fromUserString(name));
		contentList->setActive(contentList->getCount() > 1);
		curEditors.push_back(editor);
	}
}

Vector<std::pair<AssetType, String>> AssetEditorWindow::getAssetsFromFile(const Path& path) const
{
	auto assets = project.getAssetsFromFile(path);

	if (assets.empty()) {
		if (auto* handler = projectWindow.getAssetFileHandler().tryGetHandlerFor(path)) {
			auto assetId = path.replaceExtension(handler->getFileExtension());
			if (assetId.getNumberPaths() >= 3 && assetId.getParts()[0] == ".." && assetId.getParts()[1] == "halley") {
				assetId = assetId.dropFront(3);
			}
			return { std::pair<AssetType, String>(handler->getAssetType(), assetId.toString() )};
		}
	}

	return assets;
}

std::shared_ptr<AssetEditor> AssetEditorWindow::makeEditor(AssetType type)
{
	switch (type) {
	case AssetType::Animation:
	case AssetType::SpriteSheet:
	case AssetType::Texture:
		return std::make_shared<AnimationEditor>(factory, project.getGameResources(), type, project, *metadataEditor);
	case AssetType::Prefab:
	case AssetType::Scene:
		return std::make_shared<PrefabEditor>(factory, project.getGameResources(), type, project, projectWindow);
	case AssetType::UIDefinition:
		return std::make_shared<UIEditor>(factory, project.getGameResources(), project, projectWindow, projectWindow.getAPI());
	case AssetType::AudioEvent:
		return std::make_shared<AudioEventEditor>(factory, project.getGameResources(), project, projectWindow);
	case AssetType::AudioObject:
		return std::make_shared<AudioObjectEditor>(factory, project.getGameResources(), project, projectWindow);
	case AssetType::ScriptGraph:
		return std::make_shared<ScriptGraphAssetEditor>(factory, project.getGameResources(), projectWindow);
	case AssetType::RenderGraphDefinition:
		return std::make_shared<RenderGraphAssetEditor>(factory, project.getGameResources(), projectWindow);
	case AssetType::Font:
		return std::make_shared<FontEditor>(factory, project.getGameResources(), type, project, projectWindow);
	}
	return {};
}
