#include "halley/editor_extensions/choose_asset_window.h"

#include "halley/input/input_keyboard.h"
#include "halley/ui/ui_anchor.h"
#include "halley/ui/widgets/ui_list.h"
#include "halley/utils/algorithm.h"
#include "halley/ui/widgets/ui_image.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/widgets/ui_scrollbar_pane.h"
#include "halley/ui/widgets/ui_textinput.h"

using namespace Halley;


ChooseAssetWindow::ChooseAssetWindow(Vector2f minSize, UIFactory& factory, Callback callback, std::optional<String> canShowBlank)
	: UIWidget("choose_asset_window", minSize, UISizer())
	, factory(factory)
	, fuzzyMatcher(false, 100)
	, canShowBlank(std::move(canShowBlank))
{
	entries.emplace_back(DataSet{});
	entries.back().callback = std::move(callback);

	highlightCol = factory.getColourScheme()->getColour("ui_stringMatchText");

	factory.loadUI(*this, "halley/choose_asset_window");
	
	setModal(true);
	setAnchor(UIAnchor());
}

ChooseAssetWindow::~ChooseAssetWindow() = default;

void ChooseAssetWindow::onAddedToRoot(UIRoot& root)
{
	auto size = getMinimumSize();
	size.y = std::max(size.y, root.getRect().getHeight() / 2 - 50.0f);
	setMinSize(size);

	root.setFocus(getWidget("search"));
	root.registerKeyPressListener(getWidget("options"), 2);
	root.registerKeyPressListener(shared_from_this(), 1);
}

void ChooseAssetWindow::onMakeUI()
{
	options = getWidgetAs<UIList>("options");

	getWidgetAs<UITextInput>("search")->setCapturePageUpDown(false);

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

	setHandle(UIEventType::ListAccept, "options", [=](const UIEvent& event)
	{
		accept();
	});

	setHandle(UIEventType::ListSelectionChanged, "options", [=] (const UIEvent& event)
	{
		if (entries[curEntry].highlightCallback) {
			entries[curEntry].highlightCallback(event.getStringData());
		}
	});

	setChildLayerAdjustment(10);

	layout();
}

void ChooseAssetWindow::setAssetIds(Vector<String> ids, String defaultOption)
{
	setAssetIds(std::move(ids), {}, std::move(defaultOption));
}

void ChooseAssetWindow::setAssetIds(Vector<String> ids, Vector<String> names, String defaultOption)
{
	setAssetIds(std::move(ids), std::move(names), std::move(defaultOption), "", {}, {});
}

void ChooseAssetWindow::setAssetIds(Vector<String> ids, Vector<String> names, String defaultOption, String prefix, Callback callback, HighlightCallback highlightCallback)
{
	auto& e = getEntry(prefix);

	e.origIds = std::move(ids);
	e.origNames = std::move(names);
	e.defaultOption = std::move(defaultOption);
	if (e.origNames.empty()) {
		e.origNames = e.origIds;
	}
	
	if (e.origNames.size() != e.origIds.size()) {
		throw Exception("Names and ids must have same length", HalleyExceptions::UI);
	}

	if (prefix.isEmpty()) {
		setCategoryFilter("");
	} else {
		e.callback = std::move(callback);
		e.highlightCallback = std::move(highlightCallback);
	}
}

void ChooseAssetWindow::setCategoryFilter(const String& filterId)
{
	auto& e = entries[curEntry];

	if (filterId.isEmpty()) {
		ids = e.origIds;
		names = e.origNames;
	} else {
		const auto filterIter = std_ex::find_if(categoryFilters, [&] (const auto& f) { return f.id == filterId; });
		if (filterIter != categoryFilters.end()) {
			const auto& curFilter = *filterIter;

			ids.clear();
			names.clear();

			for (size_t i = 0; i < e.origIds.size(); ++i) {
				if (curFilter.matches(e.origIds[i])) {
					ids.push_back(e.origIds[i]);
					names.push_back(e.origNames[i]);
				}
			}
		} else {
			ids = e.origIds;
			names = e.origNames;
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
	const auto initialSize = options->getSize();
	
	options->clear();
	const bool hasFilter = !filter.isEmpty();
	const bool forceText = hasFilter;
	
	if (forceText) {
		options->setOrientation(UISizerType::Vertical, 1);
	} else {
		const auto cols = getNumColumns(initialSize);
		const auto orientation = cols == 1 ? UISizerType::Vertical : UISizerType::Grid;
		options->setOrientation(orientation, cols);
	}
	
	if (hasFilter) {
		for (const auto& r: fuzzyMatcher.match(filter)) {
			addItem(r.getId(), r.getString(), r.getMatchPositions());
		}
	} else {
		if (canShowBlank) {
			options->addTextItem("", LocalisedString::fromUserString(canShowBlank.value()));
		}

		if (canShowAll()) {
			Vector<std::pair<String, String>> items;
			for (size_t i = 0; i < ids.size(); ++i) {
				items.emplace_back(ids[i], names[i]);
			}
			sortItems(items);
			for (auto& item: items) {
				addItem(item.first, item.second);
			}
		} else if (!entries[curEntry].defaultOption.isEmpty()) {
			// Even on empty, show the last option selected
			const auto& defaultOption = entries[curEntry].defaultOption;
			for (size_t i = 0; i < ids.size(); ++i) {
				if (ids[i] == defaultOption) {
					addItem(defaultOption, names[i]);
				}
			}
		}
	}

	layout();
	if (hasFilter) {
		options->setSelectedOption(0);
	} else {
		options->setSelectedOptionId(entries[curEntry].defaultOption);
	}

	if (options->getCount() > 0) {
		const auto scroll = getWidgetAs<UIScrollBarPane>("optionsScroll");
		const auto itemSize = options->getItem(0)->getSize().y;
		scroll->getPane()->setScrollSpeed(itemSize);
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
		Vector<ColourOverride> overrides;
		for (const auto& p: matchPositions) {
			overrides.emplace_back(p.first, highlightCol);
			overrides.emplace_back(p.first + p.second, labelCol);
		}
		label->getTextRenderer().setColourOverride(overrides);
	}
	
	options->addItem(id, makeItemSizer(std::move(icon), std::move(label), hasSearch), 1);
}

ChooseAssetWindow::DataSet& ChooseAssetWindow::getEntry(const String& prefix)
{
	for (auto& e: entries) {
		if (e.prefix == prefix) {
			return e;
		}
	}
	entries.emplace_back(DataSet{});
	entries.back().prefix = prefix;
	return entries.back();
}

void ChooseAssetWindow::updateCurrentDataSet(const String& str)
{
	size_t longestMatch = 0;
	size_t bestIdx = 0;
	for (size_t i = 0; i < entries.size(); ++i) {
		const size_t len = entries[i].prefix.length();
		if (len > longestMatch && str.startsWith(entries[i].prefix)) {
			longestMatch = len;
			bestIdx = i;
		}
	}

	if (curEntry != bestIdx) {
		curEntry = bestIdx;
		setCategoryFilter("");
	}
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

void ChooseAssetWindow::onOptionSelected(const String& id)
{
}

void ChooseAssetWindow::sortItems(Vector<std::pair<String, String>>& items)
{
	sortItemsByName(items);
}

void ChooseAssetWindow::sortItemsByName(Vector<std::pair<String, String>>& items)
{
	std::sort(items.begin(), items.end(), [=] (const auto& a, const auto& b) { return a.second < b.second; });
}

void ChooseAssetWindow::sortItemsById(Vector<std::pair<String, String>>& items)
{
	std::sort(items.begin(), items.end(), [=] (const auto& a, const auto& b) { return a.first < b.first; });
}

size_t ChooseAssetWindow::getNumItems() const
{
	return ids.size();
}

int ChooseAssetWindow::getNumColumns(Vector2f scrollPaneSize) const
{
	return 1;
}

bool ChooseAssetWindow::isShowingDefaultDataSet() const
{
	return curEntry == 0;
}

const String& ChooseAssetWindow::getCurrentDataSetPrefix() const
{
	return entries[curEntry].prefix;
}

void ChooseAssetWindow::setCategoryFilters(Vector<AssetCategoryFilter> filters, const String& defaultOption)
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

	bindData("options", "", [=](const String& id)
	{
		onOptionSelected(id);
	});
	
	setCategoryFilter(defaultOption);
}

void ChooseAssetWindow::setSearch(String text)
{
	getWidgetAs<UITextInput>("search")->setText(text);
}

void ChooseAssetWindow::setUserFilter(const String& str)
{
	if (rawFilter != str) {
		rawFilter = str;

		const auto prevEntry = curEntry;
		updateCurrentDataSet(rawFilter);
		filter = rawFilter.mid(entries[curEntry].prefix.size());

		if (prevEntry == curEntry) {
			populateList();
		}
	}
}

void ChooseAssetWindow::setTitle(LocalisedString title)
{
	getWidgetAs<UILabel>("title")->setText(std::move(title));
}

void ChooseAssetWindow::setInputGhostText(LocalisedString text)
{
	auto search = getWidgetAs<UITextInput>("search");
	if (search) {
		search->setGhostText(text);
	}
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

	if (key.is(KeyCode::PageDown)) {
		// TODO
		return true;
	}

	if (key.is(KeyCode::PageUp)) {
		// TODO
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

UIFactory& ChooseAssetWindow::getFactory() const
{
	return factory;
}

LocalisedString ChooseAssetWindow::getItemLabel(const String& id, const String& name, bool hasSearch)
{
	return LocalisedString::fromUserString(name);
}

void ChooseAssetWindow::accept()
{
	const auto id = getWidgetAs<UIList>("options")->getSelectedOptionId();
	if (canShowBlank || !id.isEmpty()) {
		if (entries[curEntry].callback) {
			entries[curEntry].callback(id);
			entries[curEntry].callback = {};
		}

		cancelAllExcept(curEntry);
		destroy();
	}
}

void ChooseAssetWindow::cancel()
{
	cancelAllExcept(std::optional<size_t>{});
	destroy();
}

void ChooseAssetWindow::cancelAllExcept(std::optional<size_t> idx)
{
	for (size_t i = 0; i < entries.size(); ++i) {
		if (i != idx && entries[i].callback) {
			entries[i].callback({});
			entries[i].callback = {};
		}
	}
}
