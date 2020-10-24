#include "asset_editor_window.h"


#include "animation_editor.h"
#include "asset_editor.h"
#include "metadata_editor.h"
#include "prefab_editor.h"
#include "halley/tools/project/project.h"
using namespace Halley;

AssetEditorWindow::AssetEditorWindow(UIFactory& factory)
	: UIWidget("assetEditorWindow", Vector2f(), UISizer())
	, factory(factory)
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
}

void AssetEditorWindow::init(Project& project, ProjectWindow& projectWindow)
{
	this->project = &project;
	this->projectWindow = &projectWindow;
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

	if (loadedAsset != name || force) {
		loadedAsset = name;
		loadedType = type;
		
		content->clear();
		contentList->clear();
		curEditors.clear();

		if (assetSrcMode) {
			auto assets = project->getAssetsFromFile(Path(name));

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
					getWidget("contentListDropdownArea")->setActive(true);
				}

				if (clearDropdown) {
					contentListDropdown->setOptions(assetNames, 0);
				}
			}
			
			for (auto& asset: assets) {
				if (!useDropdown || asset.second == contentListDropdown->getSelectedOptionId() || std::find(assetNames.begin(), assetNames.end(), asset.second) == assetNames.end()) {
					createEditorTab(Path(name), asset.first, asset.second);
				}
			}

			if (assets.empty()) {
				metadataEditor->clear();
			} else {
				const auto type = assets.at(0).first;
				auto effectiveMeta = project->getImportMetadata(type, assets.at(0).second);
				metadataEditor->setResource(*project, type, Path(name), std::move(effectiveMeta));
			}
		} else {
			metadataEditor->clear();
			createEditorTab(Path(name), type.value(), name);
		}
	}
}

Path AssetEditorWindow::getCurrentAssetPath() const
{
	return project->getAssetsSrcPath() / loadedAsset;
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

std::shared_ptr<AssetEditor> AssetEditorWindow::makeEditor(Path filePath, AssetType type, const String& name)
{
	switch (type) {
	case AssetType::Sprite:
	case AssetType::Animation:
	case AssetType::Texture:
		return std::make_shared<AnimationEditor>(factory, project->getGameResources(), type, *project, *metadataEditor);
	case AssetType::Prefab:
	case AssetType::Scene:
		return std::make_shared<PrefabEditor>(factory, project->getGameResources(), type, *project, *projectWindow);
	}
	return {};
}
