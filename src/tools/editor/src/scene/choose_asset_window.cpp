#include "choose_asset_window.h"
#include "halley/ui/ui_anchor.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/widgets/ui_label.h"
#include "halley/ui/widgets/ui_list.h"
#include "src/ui/editor_ui_factory.h"
#include "halley/tools/project/project.h"

using namespace Halley;

ChooseAssetWindow::ChooseAssetWindow(UIFactory& factory, Callback callback, bool canShowBlank)
	: UIWidget("choose_asset_window", {}, UISizer())
	, factory(dynamic_cast<EditorUIFactory&>(factory))
	, callback(std::move(callback))
	, fuzzyMatcher(false, 100)
	, canShowBlank(canShowBlank)
{
	highlightCol = factory.getColourScheme()->getColour("ui_stringMatchText");

	makeUI();
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
	this->ids = std::move(ids);
	this->defaultOption = defaultOption;

	fuzzyMatcher.clear();
	for (auto& id: this->ids) {
		fuzzyMatcher.addString(id);
	}

	populateList();
}

void ChooseAssetWindow::populateList()
{
	options->clear();
	
	if (filter.isEmpty()) {
		if (canShowAll()) {
			if (canShowBlank) {
				options->addTextItem("", LocalisedString::fromHardcodedString("[Empty]"));
			}
			
			for (const auto& c: ids) {
				addItem(c);
			}
			
			options->layout();
			options->setSelectedOptionId(defaultOption);
		}
	} else {
		for (const auto& r: fuzzyMatcher.match(filter)) {
			addItem(r.getString(), r.getMatchPositions());
		}
		options->layout();
		options->setSelectedOption(0);
	}
}

void ChooseAssetWindow::addItem(const String& id, gsl::span<const std::pair<uint16_t, uint16_t>> matchPositions)
{
	auto sizer = std::make_shared<UISizer>();

	// Make icon
	auto icon = makeIcon(id);
	if (icon.hasMaterial()) {
		sizer->add(std::make_shared<UIImage>(icon), 0, Vector4f(0, 0, 4, 0));
	}

	// Make label
	auto label = options->makeLabel("", LocalisedString::fromUserString(id));

	// Match highlights
	auto labelCol = label->getColour();
	if (!matchPositions.empty()) {
		std::vector<ColourOverride> overrides;
		for (auto& p: matchPositions) {
			overrides.emplace_back(p.first, highlightCol);
			overrides.emplace_back(p.first + p.second, labelCol);
		}
		label->getTextRenderer().setColourOverride(overrides);
	}
	sizer->add(label, 0);
	
	options->addItem(id, sizer);
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

Sprite ChooseAssetWindow::makeIcon(const String& id)
{
	return Sprite();
}

EditorUIFactory& ChooseAssetWindow::getFactory() const
{
	return factory;
}

void ChooseAssetWindow::makeUI()
{
	add(factory.makeUI("ui/halley/choose_asset_window"), 1);

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

Sprite ChooseAssetTypeWindow::makeIcon(const String& id)
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

Sprite ChooseImportAssetWindow::makeIcon(const String& id)
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
