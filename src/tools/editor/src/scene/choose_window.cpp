#include "choose_window.h"

#include "halley/ui/ui_anchor.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/widgets/ui_label.h"
#include "halley/ui/widgets/ui_list.h"
#include "src/ui/editor_ui_factory.h"
#include "halley/tools/project/project.h"
#include "src/ui/project_window.h"

using namespace Halley;


AddComponentWindow::AddComponentWindow(UIFactory& factory, const Vector<String>& componentList, Callback callback)
	: ChooseAssetWindow(Vector2f(), factory, std::move(callback), {})
{
	setAssetIds(componentList, "");
	setTitle(LocalisedString::fromHardcodedString("Add Component"));
}


ChooseUIStyleWindow::ChooseUIStyleWindow(Vector2f minSize, UIFactory& factory, String uiClass, String defaultOption, Resources& gameResources, Callback callback)
	: ChooseAssetWindow(minSize, factory, std::move(callback), {})
{
	auto gameFactory = factory.cloneWithResources(gameResources);
	gameFactory->loadStyleSheetsFromResources();

	setAssetIds(gameFactory->getStyleSheet()->getStylesForClass(uiClass), defaultOption);
	setTitle(LocalisedString::fromHardcodedString("Choose UI Style"));
}

PaletteWindow::PaletteWindow(UIFactory& factory, Project& project, std::optional<String> initialQuery, Callback callback)
	: ChooseAssetWindow(Vector2f(), factory, std::move(callback), {})
	, project(project)
{
	auto assetNames = project.getAssetSrcList(false, Path("."), true);
	std::sort(assetNames.begin(), assetNames.end());
	
	setAssetIds(std::move(assetNames), "");
	setTitle(LocalisedString::fromHardcodedString("Open asset"));
	setAnchor(UIAnchor(Vector2f(0.5f, 0.0f), Vector2f(0.5f, 0.0f)));

	if (initialQuery) {
		setSearch(*initialQuery);
	}
}

void PaletteWindow::setIconRetriever(IconRetriever retriever)
{
	iconRetriever = std::move(retriever);
}

std::shared_ptr<UIImage> PaletteWindow::makeIcon(const String& id, bool hasSearch)
{
	const auto prefix = getCurrentDataSetPrefix();
	if (prefix != "") {
		Sprite sprite;
		if (iconRetriever) {
			sprite = iconRetriever(prefix, id);
		}
		return std::make_shared<UIImage>(sprite);
	}

	const auto type = project.getAssetImporter()->getImportAssetType(id, false);
	const auto iter = icons.find(type);
	if (iter != icons.end()) {
		return std::make_shared<UIImage>(iter->second);
	}

	auto icon = dynamic_cast<EditorUIFactory&>(getFactory()).makeImportAssetTypeIcon(type);
	icons[type] = icon;
	return std::make_shared<UIImage>(icon);
}

bool PaletteWindow::canShowAll() const
{
	return !isShowingDefaultDataSet();
}



ChooseAssetTypeWindow::ChooseAssetTypeWindow(Vector2f minSize, UIFactory& factory, AssetType type, String defaultOption, Resources& gameResources, ProjectWindow& projectWindow, bool hasPreview, std::optional<String> allowEmpty, Callback callback)
	: ChooseAssetWindow(minSize, factory, std::move(callback), allowEmpty)
	, projectWindow(projectWindow)
	, type(type)
	, hasPreview(hasPreview)
{
	auto ids = gameResources.ofType(type).enumerate();
	if (type == AssetType::Sprite) {
		for (const auto& id: factory.getColourScheme()->getSpriteNames()) {
			ids.push_back("$" + id);
		}
	}

	setAssetIds(std::move(ids), defaultOption);
	setTitle(LocalisedString::fromHardcodedString("Choose " + toString(type)));
}

std::shared_ptr<UIImage> ChooseAssetTypeWindow::makeIcon(const String& id, bool hasSearch)
{
	if (hasPreview) {
		return makePreviewIcon(id, hasSearch);
	} else {
		if (!icon.hasMaterial()) {
			icon = getFactory().makeAssetTypeIcon(type);
		}
		return std::make_shared<UIImage>(icon);
	}
}

LocalisedString ChooseAssetTypeWindow::getItemLabel(const String& id, const String& name, bool hasSearch)
{
	if (hasPreview) {
		return getPreviewItemLabel(id, name, hasSearch);
	} else {
		return ChooseAssetWindow::getItemLabel(id, name, hasSearch);
	}
}

std::shared_ptr<UISizer> ChooseAssetTypeWindow::makeItemSizer(std::shared_ptr<UIImage> uiImage, std::shared_ptr<UILabel> uiLabel, bool hasSearch)
{
	if (hasPreview) {
		return makePreviewItemSizer(uiImage, uiLabel, hasSearch);
	} else {
		return ChooseAssetWindow::makeItemSizer(uiImage, uiLabel, hasSearch);
	}
}

void ChooseAssetTypeWindow::sortItems(Vector<std::pair<String, String>>& values)
{
	if (hasPreview) {
		sortItemsById(values);
	} else {
		ChooseAssetWindow::sortItems(values);
	}
}

LocalisedString ChooseAssetTypeWindow::getPreviewItemLabel(const String& id, const String& name, bool hasSearch)
{
	if (hasSearch) {
		return LocalisedString::fromUserString(id);
	} else {
		return LocalisedString::fromUserString(Path(id).getFilename().toString());
	}
}

std::shared_ptr<UIImage> ChooseAssetTypeWindow::makePreviewIcon(const String& id, bool hasSearch)
{
	const Vector2f thumbSizeBig = Vector2f(128, 128);
	const Vector2f thumbSizeSmall = Vector2f(64, 64);
	auto thumbSize = hasSearch ? thumbSizeSmall : thumbSizeBig;
	
	if (!icon.hasMaterial()) {
		emptyPreviewIcon = Sprite().setImage(getFactory().getResources(), "whitebox.png").setColour(Colour4f(0, 0, 0, 0)).scaleTo(thumbSizeBig);
		emptyPreviewIconSmall = emptyPreviewIcon.clone().scaleTo(thumbSizeSmall);
		icon = getFactory().makeAssetTypeIcon(type);
	}
	auto image = std::make_shared<UIImage>(hasSearch ? emptyPreviewIconSmall : emptyPreviewIcon);
	auto imageWeak = std::weak_ptr(image);
	
	image->addBehaviour(std::make_shared<UIImageVisibleBehaviour>([imageWeak, this, id, thumbSize] (UIImage& img)
	{
		if (auto future = projectWindow.getAssetPreviewData(type, id, Vector2i(thumbSize)); future.isValid()) {
			future.then(Executors::getMainUpdateThread(), [imageWeak, thumbSize] (AssetPreviewData data)
			{
				if (auto image = imageWeak.lock(); image) {
					auto sprite = std::move(data.sprite);
					if (sprite.hasMaterial()) {
						sprite.scaleTo(thumbSize);
						image->setSprite(std::move(sprite));
					}
				}
			});
		}
	}, [=] (UIImage& img)
	{
		// On invisible
		img.setSprite(hasSearch ? emptyPreviewIconSmall : emptyPreviewIcon);
	}));
	
	return image;
}

std::shared_ptr<UISizer> ChooseAssetTypeWindow::makePreviewItemSizer(std::shared_ptr<UIImage> icon, std::shared_ptr<UILabel> label, bool hasSearch)
{
	if (hasSearch) {
		return ChooseAssetWindow::makeItemSizer(std::move(icon), std::move(label), hasSearch);
	} else {
		label->setMaxWidth(128.0f);
		return makeItemSizerBigIcon(std::move(icon), std::move(label));
	}
}

int ChooseAssetTypeWindow::getNumColumns(Vector2f scrollPaneSize) const
{
	return hasPreview ? static_cast<int>(std::floor(scrollPaneSize.x / 150.0f)) : 1;
}


ChoosePrefabWindow::ChoosePrefabWindow(UIFactory& factory, std::optional<String> defaultOption, Resources& gameResources, ProjectWindow& projectWindow, Callback callback)
	: ChooseAssetTypeWindow(projectWindow.getChoosePrefabWindowSize(), factory, AssetType::Prefab, defaultOption.value_or(projectWindow.getSetting(EditorSettingType::Project, lastOptionKey).asString("")), gameResources, projectWindow, true, {}, std::move(callback))
{
	const auto lastCategory = projectWindow.getSetting(EditorSettingType::Project, lastCategoryKey).asString("");
	setCategoryFilters(projectWindow.getAssetPreviewGenerator().getPrefabCategoryFilters(), lastCategory);
}

void ChoosePrefabWindow::onCategorySet(const String& id)
{
	projectWindow.setSetting(EditorSettingType::Project, lastCategoryKey, ConfigNode(id));
}

void ChoosePrefabWindow::onOptionSelected(const String& id)
{
	lastOption = id;
}

bool ChoosePrefabWindow::onDestroyRequested()
{
	projectWindow.setSetting(EditorSettingType::Project, lastOptionKey, ConfigNode(lastOption));
	return true;
}



ChooseEntityWindow::ChooseEntityWindow(UIFactory& factory, const Vector<IEntityEditorCallbacks::EntityInfo>& entities, Callback callback)
	: ChooseAssetWindow(Vector2f(), factory, std::move(callback), "[None]")
{
	Vector<String> ids;
	Vector<String> names;
	for (auto& e: entities) {
		ids.push_back(e.uuid.toString());
		names.push_back(e.name);
		icons[ids.back()] = e.icon;
	}

	setAssetIds(ids, names, "");
	setTitle(LocalisedString::fromHardcodedString("Choose Entity"));
}

int ChooseEntityWindow::getNumColumns(Vector2f scrollPaneSize) const
{
	return 1;
}

std::shared_ptr<UIImage> ChooseEntityWindow::makeIcon(const String& id, bool hasSearch)
{
	const auto iter = icons.find(id);
	if (iter != icons.end()) {
		return std::make_shared<UIImage>(iter->second);
	}
	return {};
}

std::shared_ptr<UISizer> ChooseEntityWindow::makeItemSizer(std::shared_ptr<UIImage> icon, std::shared_ptr<UILabel> label, bool hasSearch)
{
	return ChooseAssetWindow::makeItemSizer(icon, label, true);
}
