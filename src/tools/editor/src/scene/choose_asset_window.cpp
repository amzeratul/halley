#include "choose_asset_window.h"

#include "scene_editor_window.h"
#include "halley/ui/ui_anchor.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/widgets/ui_label.h"
#include "halley/ui/widgets/ui_list.h"
#include "src/ui/editor_ui_factory.h"
#include "halley/tools/project/project.h"

using namespace Halley;

bool ChooseAssetWindow::CategoryFilter::matches(const String& id) const
{
	for (const auto& prefix: prefixes) {
		if (id.startsWith(prefix)) {
			return true;
		}
	}
	return false;
}

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

	factory.loadUI(*this, "ui/halley/choose_asset_window");
	
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
			const auto& filter = *filterIter;

			ids.clear();
			names.clear();

			for (size_t i = 0; i < ids.size(); ++i) {
				if (filter.matches(ids[i])) {
					ids.push_back(ids[i]);
					names.push_back(names[i]);
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
}

void ChooseAssetWindow::populateList()
{
	options->clear();
	
	if (filter.isEmpty()) {
		options->setOrientation(orientation, nColumns);
		if (canShowAll()) {
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
			
			options->layout();
			options->setSelectedOptionId(defaultOption);
		}
	} else {
		options->setOrientation(UISizerType::Vertical, 1);
		for (const auto& r: fuzzyMatcher.match(filter)) {
			addItem(r.getId(), r.getString(), r.getMatchPositions());
		}
		options->layout();
		options->setSelectedOption(0);
	}
}

void ChooseAssetWindow::addItem(const String& id, const String& name, gsl::span<const std::pair<uint16_t, uint16_t>> matchPositions)
{
	const bool hasSearch = !matchPositions.empty();
	
	// Make icon
	auto icon = makeIcon(id, hasSearch);

	// Make label
	auto label = options->makeLabel("", LocalisedString::fromUserString(name));
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

std::shared_ptr<UISizer> ChooseAssetWindow::makeItemSizer(Sprite icon, std::shared_ptr<UILabel> label, bool hasSearch)
{
	auto sizer = std::make_shared<UISizer>();
	if (icon.hasMaterial()) {
		sizer->add(std::make_shared<UIImage>(icon), 0, Vector4f(0, 0, 4, 0));
	}
	sizer->add(label, 0);
	return sizer;
}

std::shared_ptr<UISizer> ChooseAssetWindow::makeItemSizerBigIcon(Sprite icon, std::shared_ptr<UILabel> label)
{
	auto sizer = std::make_shared<UISizer>(UISizerType::Vertical);
	if (icon.hasMaterial()) {
		sizer->add(std::make_shared<UIImage>(icon), 0, Vector4f(0, 0, 4, 0), UISizerAlignFlags::CentreHorizontal);
	}
	sizer->add(label, 0, {}, UISizerAlignFlags::CentreHorizontal);
	return sizer;
}

void ChooseAssetWindow::sortItems(std::vector<std::pair<String, String>>& items)
{
	std::sort(items.begin(), items.end(), [=] (const auto& a, const auto& b) { return a.second < b.second; });
}

void ChooseAssetWindow::setCategoryFilters(std::vector<CategoryFilter> filters)
{
	categoryFilters = std::move(filters);
	
	getWidget("tabsContainer")->setActive(true);

	auto tabs = getWidgetAs<UIList>("tabs");
	tabs->addTextItemAligned("", LocalisedString::fromHardcodedString("All"));

	for (auto& filter: categoryFilters) {
		auto item = tabs->addTextIconItem(filter.id, filter.showName ? filter.name : LocalisedString(), filter.icon);
		item->setToolTip(filter.name);
	}

	setHandle(UIEventType::ListSelectionChanged, "tabs", [=] (const UIEvent& event)
	{
		setCategoryFilter(event.getStringData());
	});
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

Sprite ChooseAssetWindow::makeIcon(const String& id, bool hasSearch)
{
	return Sprite();
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

ChooseAssetTypeWindow::ChooseAssetTypeWindow(UIFactory& factory, AssetType type, String defaultOption, Resources& gameResources, Callback callback)
	: ChooseAssetWindow(factory, std::move(callback))
	, type(type)
{
	setAssetIds(gameResources.ofType(type).enumerate(), defaultOption);
	setTitle(LocalisedString::fromHardcodedString("Choose " + toString(type)));
}

Sprite ChooseAssetTypeWindow::makeIcon(const String& id, bool hasSearch)
{
	if (!icon.hasMaterial()) {
		icon = getFactory().makeAssetTypeIcon(type);
	}
	return icon;
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

Sprite ChooseImportAssetWindow::makeIcon(const String& id, bool hasSearch)
{
	const auto type = project.getAssetImporter()->getImportAssetType(id, false);
	const auto iter = icons.find(type);
	if (iter != icons.end()) {
		return iter->second;
	}

	auto icon = getFactory().makeImportAssetTypeIcon(type);
	icons[type] = icon;
	return icon;
}

bool ChooseImportAssetWindow::canShowAll() const
{
	return false;
}

ChoosePrefabWindow::ChoosePrefabWindow(UIFactory& factory, String defaultOption, Resources& gameResources, std::vector<CategoryFilter> categories, Callback callback)
	: ChooseAssetWindow(factory, std::move(callback), false, UISizerType::Vertical, 1)
{
	setAssetIds(gameResources.ofType(AssetType::Prefab).enumerate(), std::move(defaultOption));
	setTitle(LocalisedString::fromHardcodedString("Choose Prefab"));
	setCategoryFilters(std::move(categories));
}

Sprite ChoosePrefabWindow::makeIcon(const String& id, bool hasSearch)
{
	if (!icon.hasMaterial()) {
		icon = getFactory().makeAssetTypeIcon(AssetType::Prefab);
	}
	return icon;
}
