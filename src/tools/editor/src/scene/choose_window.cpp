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
	: ChooseAssetWindow(Vector2f(), factory, std::move(callback), false)
{
	setAssetIds(componentList, "");
	setTitle(LocalisedString::fromHardcodedString("Add Component"));
}



ChooseImportAssetWindow::ChooseImportAssetWindow(UIFactory& factory, Project& project, Callback callback)
	: ChooseAssetWindow(Vector2f(), factory, std::move(callback), false)
	, project(project)
{
	auto assetNames = project.getAssetSrcList();
	std::sort(assetNames.begin(), assetNames.end());
	
	setAssetIds(std::move(assetNames), "");
	setTitle(LocalisedString::fromHardcodedString("Open asset"));
}

std::shared_ptr<UIImage> ChooseImportAssetWindow::makeIcon(const String& id, bool hasSearch)
{
	const auto type = project.getAssetImporter()->getImportAssetType(id, false);
	const auto iter = icons.find(type);
	if (iter != icons.end()) {
		return std::make_shared<UIImage>(iter->second);
	}

	auto icon = dynamic_cast<EditorUIFactory&>(getFactory()).makeImportAssetTypeIcon(type);
	icons[type] = icon;
	return std::make_shared<UIImage>(icon);
}

bool ChooseImportAssetWindow::canShowAll() const
{
	return false;
}



ChooseAssetTypeWindow::ChooseAssetTypeWindow(Vector2f minSize, UIFactory& factory, AssetType type, String defaultOption, Resources& gameResources, ProjectWindow& projectWindow, bool hasPreview, Callback callback)
	: ChooseAssetWindow(minSize, factory, std::move(callback), false)
	, projectWindow(projectWindow)
	, type(type)
	, hasPreview(hasPreview)
{
	setAssetIds(gameResources.ofType(type).enumerate(), defaultOption);
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
		label->setMaxWidth(128);
		return makeItemSizerBigIcon(std::move(icon), std::move(label));
	}
}

int ChooseAssetTypeWindow::getNumColumns(Vector2f scrollPaneSize) const
{
	return hasPreview ? static_cast<int>(std::floor(scrollPaneSize.x / 150.0f)) : 1;
}


ChoosePrefabWindow::ChoosePrefabWindow(UIFactory& factory, String defaultOption, Resources& gameResources, ProjectWindow& projectWindow, Callback callback)
	: ChooseAssetTypeWindow(projectWindow.getChoosePrefabWindowSize(), factory, AssetType::Prefab, defaultOption, gameResources, projectWindow, true, std::move(callback))
{
	const auto lastCategory = projectWindow.getSetting(EditorSettingType::Project, lastCategoryKey).asString("");
	setCategoryFilters(projectWindow.getAssetPreviewGenerator().getPrefabCategoryFilters(), lastCategory);
}

void ChoosePrefabWindow::onCategorySet(const String& id)
{
	projectWindow.setSetting(EditorSettingType::Project, lastCategoryKey, ConfigNode(id));
}
