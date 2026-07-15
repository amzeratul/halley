#include "localisation_grid.h"

#include "localisation_filters.h"

using namespace Halley;

LocalisationGrid::LocalisationGrid(UIFactory& factory, const HalleyAPI& api, LocalisationFilterRules filterRules, ILocalisationGridListener& listener)
	: UIGrid("localisation_grid", factory)
	, factory(factory)
	, api(api)
	, filterRules(std::move(filterRules))
	, listener(listener)
{
	outdatedCol = factory.getColourScheme()->getColour("ui_logWarningText");

	priorityIcons[LocPriority::Lowest] = Sprite().setImage(factory.getResources(), "ui/priority_lowest.png").setColour(Colour4f::fromHexString("#3930d2"));
	priorityIcons[LocPriority::Low] = Sprite().setImage(factory.getResources(), "ui/priority_low.png").setColour(Colour4f::fromHexString("#00abdf"));
	priorityIcons[LocPriority::Normal] = Sprite().setImage(factory.getResources(), "ui/priority_normal.png").setColour(Colour4f::fromHexString("#40bc2a"));
	priorityIcons[LocPriority::High] = Sprite().setImage(factory.getResources(), "ui/priority_high.png").setColour(Colour4f::fromHexString("#ffbf00"));
	priorityIcons[LocPriority::Highest] = Sprite().setImage(factory.getResources(), "ui/priority_highest.png").setColour(Colour4f::fromHexString("#df3434"));
	contextIcon = Sprite().setImage(factory.getResources(), "ui/loc_context.png").setColour(Colour4f::fromHexString("#ffffff"));
	commentIcon = Sprite().setImage(factory.getResources(), "ui/loc_comment.png").setColour(Colour4f::fromHexString("#ffffff"));
	readyIcon = Sprite().setImage(factory.getResources(), "ui/loc_ready.png").setColour(Colour4f::fromHexString("#15BD2B"));
}

size_t LocalisationGrid::getSrcRowCount() const
{
	return origData ? origData->getNumEntries() : 0;
}

std::pair<Vector<float>, Vector<String>> LocalisationGrid::getColumns() const
{
	Vector<float> columns;
	Vector<String> columnNames;

	const float width = getSize().x - 1;
	const float numWidth = 50;
	const float fileWidth = 150;
	const float keyWidth = 250;
	const float priorityWidth = 30;
	const float readyWidth = 30;
	const float commentWidth = 30;
	const float contextWidth = 30;
	const float fixedWidth = numWidth + fileWidth + keyWidth + (showProperties ? readyWidth + priorityWidth + commentWidth + contextWidth : 0);
	columns.push_back(numWidth);
	columnNames.push_back("#");
	columns.push_back(fileWidth);
	columnNames.push_back("Group");
	columns.push_back(keyWidth);
	columnNames.push_back("Key");

	const float dynamicWidth = std::floor((width - fixedWidth) / (translatedData && origData ? 2 : 1));
	if (origData) {
		columns.push_back(dynamicWidth);
		columnNames.push_back("Original");
	}

	if (translatedData) {
		columns.push_back(dynamicWidth);
		columnNames.push_back("Translated");
	}

	if (showProperties) {
		columns.push_back(readyWidth);
		columnNames.push_back("Rdy");
		columns.push_back(priorityWidth);
		columnNames.push_back("Pri");
		columns.push_back(commentWidth);
		columnNames.push_back("Cmt");
		columns.push_back(contextWidth);
		columnNames.push_back("Ctx");
	}

	return { columns, columnNames };
}

void LocalisationGrid::getLineDrawData(int idx, Vector<String>& strs, Vector<Colour4f>& colours, Vector<Sprite>& sprites) const
{
	if (origData) {
		const auto& entry = origData->getEntry(idx);
		const auto& groupName = origData->getGroupNameEntry(idx);
		const size_t groupSlashPos = groupName.find_last_of('/');
		const auto shortGroupName = groupSlashPos == std::string::npos ? groupName : groupName.mid(groupSlashPos + 1);

		size_t numIcons = (showProperties ? 4 : 0);
		size_t len = 4 + (translatedData ? 1 : 0) + numIcons;
		size_t firstIcon = len - numIcons;
		strs.resize(len);
		colours.resize(len, textCol);
		sprites.resize(len);

		strs[0] = toString(idx + 1);
		strs[1] = shortGroupName;
		strs[2] = entry.getKey();
		strs[3] = entry.getValue();

		if (translatedData) {
			if (auto* translatedEntry = translatedData->tryGetEntry(entry.getKey())) {
				strs[4] = translatedEntry->getValue();
				colours[4] = entry.getVersion() == translatedEntry->origVersion ? textCol : outdatedCol;
			}
		}

		if (showProperties) {
			if (isReadyToTranslate(entry)) {
				sprites[firstIcon] = readyIcon;
			}
			sprites[firstIcon + 1] = priorityIcons.at(entry.getPriority());
			if (!entry.getComment().isEmpty()) {
				sprites[firstIcon + 2] = commentIcon;
			}
			if (!entry.getContext().isEmpty()) {
				sprites[firstIcon + 3] = contextIcon;
			}
		}
	}
}

void LocalisationGrid::onRightClick(std::optional<int> line)
{
	if (!line) {
		return;
	}

	auto menuOptions = Vector<UIPopupMenuItem>();

	menuOptions.push_back(UIPopupMenuItem("copyKey", LocalisedString::fromHardcodedString("Copy Key"), {}));
	menuOptions.push_back(UIPopupMenuItem("copyOriginal", LocalisedString::fromHardcodedString("Copy Original String"), {}));
	if (translatedData) {
		menuOptions.push_back(UIPopupMenuItem("copyTranslation", LocalisedString::fromHardcodedString("Copy Translated String"), {}));
	}

	auto menu = std::make_shared<UIPopupMenu>("loc_grid_context_menu", factory.getStyle("popupMenu"), menuOptions);
	menu->spawnOnRoot(*getRoot());

	menu->setHandle(UIEventType::PopupAccept, [this, line] (const UIEvent& e) {
		const auto& choice = e.getStringData();
		const auto& entry = origData->getEntry(*line);

		if (choice == "copyKey") {
			sendToClipboard(entry.getKey());
		} else if (choice == "copyOriginal") {
			sendToClipboard(entry.getValue());
		} else if (choice == "copyTranslation") {
			if (auto* translated = translatedData ? translatedData->tryGetEntry(entry.getKey()) : nullptr) {
				sendToClipboard(translated->getValue());
			}
		}
	});
}

void LocalisationGrid::copySelection()
{
	if (getSelectedLines().empty()) {
		return;
	}

	ConfigNode result;
	Vector<String> keyOrder;

	for (const auto row: getSelectedLines()) {
		const auto& entry = origData->getEntry(row);
		result[entry.getKey()] = entry.getValue();
		keyOrder += entry.getKey();
	}

	YAMLConvert::EmitOptions options;
	options.mapKeyOrder = keyOrder;
	sendToClipboard(YAMLConvert::generateYAML(result, options));
}

void LocalisationGrid::sendToClipboard(const String& str)
{
	if (const auto clipboard = api.system->getClipboard()) {
		clipboard->setData(str);
	}
}

bool LocalisationGrid::isReadyToTranslate(const LocalisationDataEntry& entry) const
{
	return entry.getReadyState(filterRules, language) == LocReadyStatus::Ready;
}

const String& LocalisationGrid::getKeyAt(int idx) const
{
	if (origData && idx >= 0 && idx < static_cast<int>(getSrcRowCount())) {
		return origData->getEntry(idx).getKey();
	} else {
		return String::emptyString();
	}
}

void LocalisationGrid::setData(const ILocOriginalData* origData, LocTranslationData* translatedData, bool showProperties, I18NLanguage language)
{
	this->origData = origData;
	this->translatedData = translatedData;
	this->showProperties = showProperties;
	this->language = language;

	onDataUpdated();
}

String LocalisationGrid::getCellToolTip(int row, int col, const String& colName) const
{
	const auto& entry = origData->getEntry(row);
	if (colName == "Group") {
		return origData->getGroupNameEntry(row);
	} else if (colName == "Key") {
		String tooltip = entry.getKey() + "\nv" + toString(entry.getVersion());

		if (translatedData) {
			if (const auto* translatedEntry = translatedData->tryGetEntry(entry.getKey())) {
				if (translatedEntry->origVersion != entry.getVersion()) {
					tooltip += " (translation at v" + toString(translatedEntry->origVersion) + ")";
				}
			}
		}

		Logger::logInfo(tooltip);
		return tooltip;
	} else if (colName == "Original") {
		return entry.getValue();
	} else if (colName == "Translated") {
		if (const auto* translatedEntry = translatedData->tryGetEntry(entry.getKey())) {
			return translatedEntry->getValue();
		}
	} else if (colName == "Rdy") {
		return isReadyToTranslate(entry) ? "Ready to Translate" : "Not Ready to Translate";
	} else if (colName == "Pri") {
		return "Priority: " + toString(entry.getPriority());
	} else if (colName == "Cmt") {
		return entry.getComment();
	} else if (colName == "Ctx") {
		return entry.getContext();
	}
	return {};
}

std::optional<MouseCursorMode> LocalisationGrid::getMouseAtCell(int row, int col) const
{
	const auto& entry = origData->getEntry(row);
	if (columnNames[col] == "Ctx") {
		if (!entry.getContext().isEmpty()) {
			return MouseCursorMode::Hand;
		}
	}
	return {};
}

bool LocalisationGrid::onClickCell(int row, int col, KeyMods mods)
{
	const auto& entry = origData->getEntry(row);
	if (columnNames[col] == "Ctx") {
		if (!entry.getContext().isEmpty()) {
			listener.openContext(entry.getContext(), false);
			lastContextSent = row;
			return false; // Let the line be selected anyway
		}
	}
	return false;
}

void LocalisationGrid::onSelectionChanged()
{
	const auto sel = getActiveSelectedLine();
	if (sel >= 0 && sel != lastContextSent) {
		const auto& entry = origData->getEntry(sel);
		if (!entry.getContext().isEmpty()) {
			listener.openContext(entry.getContext(), true);
		}
	}
	lastContextSent = {};
}
