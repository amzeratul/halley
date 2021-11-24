#include "choose_asset_window.h"

#include "halley/ui/ui_anchor.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/widgets/ui_label.h"
#include "halley/ui/widgets/ui_list.h"
#include "src/ui/editor_ui_factory.h"
#include "halley/tools/project/project.h"
#include "src/ui/project_window.h"

using namespace Halley;

ChooseAssetWindow::ChooseAssetWindow(UIFactory& factory, Callback callback, bool canShowBlank, UISizerType orientation, int nColumns)
	: UIWidget("choose_asset_window", {}, UISizer())
	, factory(dynamic_cast<EditorUIFactory&>(factory))
	, callback(std::move(callback))
	, orientation(orientation)
	, nColumns(nColumns)
	, fuzzyMatcher(false, 100)
	, canShowBlank(canShowBlank)
{
	highlightCol = factory.getColourScheme()->getColour("ui_stringMatchText");

	factory.loadUI(*this, "halley/choose_asset_window");
	
	setModal(true);
	setAnchor(UIAnchor());
}

ChooseAssetWindow::~ChooseAssetWindow() = default;

void ChooseAssetWindow::onAddedToRoot(UIRoot& root)
{
	root.setFocus(getWidget("search"));
	root.registerKeyPressListener(getWidget("options"), 2);
	root.registerKeyPressListener(shared_from_this(), 1);
}

void ChooseAssetWindow::setAssetIds(std::vector<String> ids, String defaultOption)
{
	setAssetIds(std::move(ids), {}, std::move(defaultOption));
}

void ChooseAssetWindow::setAssetIds(std::vector<String> _ids, std::vector<String> _names, String _defaultOption)
{
	origIds = std::move(_ids);
	origNames = std::move(_names);
	defaultOption = std::move(_defaultOption);
	if (origNames.empty()) {
		origNames = origIds;
	}
	
	if (origNames.size() != origIds.size()) {
		throw Exception("Names and ids must have same length", HalleyExceptions::UI);
	}

	setCategoryFilter("");
}

void ChooseAssetWindow::setCategoryFilter(const String& filterId)
{
	if (filterId.isEmpty()) {
		ids = origIds;
		names = origNames;
	} else {
		const auto filterIter = std_ex::find_if(categoryFilters, [&] (const auto& f) { return f.id == filterId; });
		if (filterIter != categoryFilters.end()) {
			const auto& curFilter = *filterIter;

			ids.clear();
			names.clear();

			for (size_t i = 0; i < origIds.size(); ++i) {
				if (curFilter.matches(origIds[i])) {
					ids.push_back(origIds[i]);
					names.push_back(origNames[i]);
				}
			}
		} else {
			ids = origIds;
			names = origNames;
		}
	}
	
	fuzzyMatcher.clear();
	for (size_t i = 0; i < ids.size(); ++i) {
		fuzzyMatcher.addString(names[i], ids[i]);
	}

	populateList();

	onCategorySet(filterId);
}

void ChooseAssetWindow::populateList()
{
	options->clear();
	const bool hasFilter = !filter.isEmpty();
	const bool forceText = hasFilter;
	
	if (forceText) {
		options->setOrientation(UISizerType::Vertical, 1);
	} else {
		options->setOrientation(orientation, nColumns);
	}
	
	if (hasFilter) {
		for (const auto& r: fuzzyMatcher.match(filter)) {
			addItem(r.getId(), r.getString(), r.getMatchPositions());
		}			
	} else if (canShowAll()) {
		if (canShowBlank) {
			options->addTextItem("", LocalisedString::fromHardcodedString("[Empty]"));
		}

		std::vector<std::pair<String, String>> items;
		for (size_t i = 0; i < ids.size(); ++i) {
			items.emplace_back(ids[i], names[i]);
		}
		sortItems(items);
		for (auto& item: items) {
			addItem(item.first, item.second);
		}
	}

	options->layout();
	if (hasFilter) {
		options->setSelectedOption(0);
	} else {
		options->setSelectedOptionId(defaultOption);
	}
}

void ChooseAssetWindow::addItem(const String& id, const String& name, gsl::span<const std::pair<uint16_t, uint16_t>> matchPositions)
{
	const bool hasSearch = !matchPositions.empty();
	
	// Make icon
	auto icon = makeIcon(id, hasSearch);

	// Make label
	auto label = options->makeLabel("", getItemLabel(id, name, hasSearch));
	auto labelCol = label->getColour();
	if (!matchPositions.empty()) {
		std::vector<ColourOverride> overrides;
		for (const auto& p: matchPositions) {
			overrides.emplace_back(p.first, highlightCol);
			overrides.emplace_back(p.first + p.second, labelCol);
		}
		label->getTextRenderer().setColourOverride(overrides);
	}
	
	options->addItem(id, makeItemSizer(std::move(icon), std::move(label), hasSearch), 1);
}

std::shared_ptr<UISizer> ChooseAssetWindow::makeItemSizer(std::shared_ptr<UIImage> icon, std::shared_ptr<UILabel> label, bool hasSearch)
{
	auto sizer = std::make_shared<UISizer>();
	if (icon) {
		sizer->add(icon, 0, Vector4f(0, 0, 4, 0));
	}
	sizer->add(label, 0);
	return sizer;
}

std::shared_ptr<UISizer> ChooseAssetWindow::makeItemSizerBigIcon(std::shared_ptr<UIImage> icon, std::shared_ptr<UILabel> label)
{
	auto sizer = std::make_shared<UISizer>(UISizerType::Vertical);
	if (icon) {
		sizer->add(icon, 0, Vector4f(0, 0, 4, 0), UISizerAlignFlags::CentreHorizontal);
	}
	sizer->add(label, 0, {}, UISizerAlignFlags::CentreHorizontal);
	return sizer;
}

void ChooseAssetWindow::onCategorySet(const String& id)
{
}

void ChooseAssetWindow::sortItems(std::vector<std::pair<String, String>>& items)
{
	sortItemsByName(items);
}

void ChooseAssetWindow::sortItemsByName(std::vector<std::pair<String, String>>& items)
{
	std::sort(items.begin(), items.end(), [=] (const auto& a, const auto& b) { return a.second < b.second; });
}

void ChooseAssetWindow::sortItemsById(std::vector<std::pair<String, String>>& items)
{
	std::sort(items.begin(), items.end(), [=] (const auto& a, const auto& b) { return a.first < b.first; });
}

void ChooseAssetWindow::setCategoryFilters(std::vector<AssetCategoryFilter> filters, const String& defaultOption)
{
	categoryFilters = std::move(filters);
	
	getWidget("tabsContainer")->setActive(true);

	auto tabs = getWidgetAs<UIList>("tabs");
	tabs->addTextItemAligned("", LocalisedString::fromHardcodedString("All"));

	for (const auto& filter: categoryFilters) {
		auto item = tabs->addTextIconItem(filter.id, filter.showName ? filter.name : LocalisedString(), filter.icon, -1, {}, UISizerAlignFlags::Centre, filter.name);
	}

	bindData("tabs", defaultOption, [=] (const String& id)
	{
		setCategoryFilter(id);
	});
	setCategoryFilter(defaultOption);
}

void ChooseAssetWindow::setUserFilter(const String& str)
{
	if (filter != str) {
		filter = str;
		populateList();
	}
}

void ChooseAssetWindow::setTitle(LocalisedString title)
{
	getWidgetAs<UILabel>("title")->setText(std::move(title));
}

bool ChooseAssetWindow::onKeyPress(KeyboardKeyPress key)
{
	if (key.is(KeyCode::Enter)) {
		accept();
		return true;
	}

	if (key.is(KeyCode::Esc)) {
		cancel();
		return true;
	}

	return false;
}

bool ChooseAssetWindow::canShowAll() const
{
	return true;
}

std::shared_ptr<UIImage> ChooseAssetWindow::makeIcon(const String& id, bool hasSearch)
{
	return {};
}

EditorUIFactory& ChooseAssetWindow::getFactory() const
{
	return factory;
}

void ChooseAssetWindow::onMakeUI()
{
	options = getWidgetAs<UIList>("options");

	setHandle(UIEventType::ButtonClicked, "ok", [=] (const UIEvent& event)
	{
		accept();
	});

	setHandle(UIEventType::ButtonClicked, "cancel", [=](const UIEvent& event)
	{
		cancel();
	});

	setHandle(UIEventType::TextChanged, "search", [=](const UIEvent& event)
	{
		setUserFilter(event.getStringData());
	});

	setHandle(UIEventType::TextSubmit, "search", [=](const UIEvent& event)
	{
		accept();
	});

	setChildLayerAdjustment(10);
}

LocalisedString ChooseAssetWindow::getItemLabel(const String& id, const String& name, bool hasSearch)
{
	return LocalisedString::fromUserString(name);
}

void ChooseAssetWindow::accept()
{
	const auto id = getWidgetAs<UIList>("options")->getSelectedOptionId();
	if (canShowBlank || !id.isEmpty()) {
		if (callback) {
			callback(id);
			destroy();
		}
		callback = {};
	}
}

void ChooseAssetWindow::cancel()
{
	callback({});
	destroy();
}




AddComponentWindow::AddComponentWindow(UIFactory& factory, const std::vector<String>& componentList, Callback callback)
	: ChooseAssetWindow(factory, std::move(callback), false)
{
	setAssetIds(componentList, "");
	setTitle(LocalisedString::fromHardcodedString("Add Component"));
}



ChooseImportAssetWindow::ChooseImportAssetWindow(UIFactory& factory, Project& project, Callback callback)
	: ChooseAssetWindow(factory, std::move(callback), false)
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

	auto icon = getFactory().makeImportAssetTypeIcon(type);
	icons[type] = icon;
	return std::make_shared<UIImage>(icon);
}

bool ChooseImportAssetWindow::canShowAll() const
{
	return false;
}



ChooseAssetTypeWindow::ChooseAssetTypeWindow(UIFactory& factory, AssetType type, String defaultOption, Resources& gameResources, ProjectWindow& projectWindow, bool hasPreview, Callback callback)
	: ChooseAssetWindow(factory, std::move(callback), false, UISizerType::Grid, hasPreview ? 4 : 1)
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

void ChooseAssetTypeWindow::sortItems(std::vector<std::pair<String, String>>& values)
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
			future.then(Executors::getMainThread(), [imageWeak, thumbSize] (AssetPreviewData data)
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




ChoosePrefabWindow::ChoosePrefabWindow(UIFactory& factory, String defaultOption, Resources& gameResources, ProjectWindow& projectWindow, Callback callback)
	: ChooseAssetTypeWindow(factory, AssetType::Prefab, defaultOption, gameResources, projectWindow, true, std::move(callback))
{
	const auto lastCategory = projectWindow.getSetting(EditorSettingType::Project, lastCategoryKey).asString("");
	setCategoryFilters(projectWindow.getAssetPreviewGenerator().getPrefabCategoryFilters(), lastCategory);
}

void ChoosePrefabWindow::onCategorySet(const String& id)
{
	projectWindow.setSetting(EditorSettingType::Project, lastCategoryKey, ConfigNode(id));
}
