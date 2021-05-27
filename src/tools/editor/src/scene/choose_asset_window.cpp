#include "choose_asset_window.h"
#include "halley/ui/ui_anchor.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/widgets/ui_label.h"
#include "halley/ui/widgets/ui_list.h"
#include "src/ui/editor_ui_factory.h"
#include "halley/tools/project/project.h"

using namespace Halley;

ChooseAssetWindow::ChooseAssetWindow(UIFactory& factory, Callback callback, bool canShowBlank, UISizerType orientation, int nColumns)
	: UIWidget("choose_asset_window", {}, UISizer())
	, factory(dynamic_cast<EditorUIFactory&>(factory))
	, callback(std::move(callback))
	, fuzzyMatcher(false, 100)
	, canShowBlank(canShowBlank)
	, orientation(orientation)
	, nColumns(nColumns)
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
	ids = std::move(_ids);
	names = std::move(_names);
	defaultOption = std::move(_defaultOption);

	effectiveNames = &(names.empty() ? ids : names);
	if (effectiveNames->size() != ids.size()) {
		throw Exception("Names and ids must have same length", HalleyExceptions::UI);
	}

	fuzzyMatcher.clear();
	for (size_t i = 0; i < ids.size(); ++i) {
		fuzzyMatcher.addString((*effectiveNames)[i], ids[i]);
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
				items.emplace_back(ids[i], (*effectiveNames)[i]);
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

void ChooseAssetWindow::setFilter(const String& str)
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
		setFilter(event.getStringData());
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
