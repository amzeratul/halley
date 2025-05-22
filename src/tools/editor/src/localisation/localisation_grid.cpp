#include "localisation_grid.h"

using namespace Halley;

LocalisationGrid::LocalisationGrid(UIFactory& factory, const HalleyAPI& api)
	: UIGrid("localisation_grid", factory)
	, factory(factory)
	, api(api)
{
	outdatedCol = factory.getColourScheme()->getColour("ui_logWarningText");

	priorityIcons[LocPriority::Lowest] = Sprite().setImage(factory.getResources(), "ui/priority_lowest.png").setColour(Colour4f::fromHexString("#3930d2"));
	priorityIcons[LocPriority::Low] = Sprite().setImage(factory.getResources(), "ui/priority_low.png").setColour(Colour4f::fromHexString("#00abdf"));
	priorityIcons[LocPriority::Normal] = Sprite().setImage(factory.getResources(), "ui/priority_normal.png").setColour(Colour4f::fromHexString("#40bc2a"));
	priorityIcons[LocPriority::High] = Sprite().setImage(factory.getResources(), "ui/priority_high.png").setColour(Colour4f::fromHexString("#ffbf00"));
	priorityIcons[LocPriority::Highest] = Sprite().setImage(factory.getResources(), "ui/priority_highest.png").setColour(Colour4f::fromHexString("#df3434"));
	contextIcon = Sprite().setImage(factory.getResources(), "ui/loc_context.png").setColour(Colour4f::fromHexString("#ffffff"));
	commentIcon = Sprite().setImage(factory.getResources(), "ui/loc_comment.png").setColour(Colour4f::fromHexString("#ffffff"));
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
	const float keyWidth = 250;
	const float priorityWidth = 30;
	const float commentWidth = 30;
	const float contextWidth = 30;
	const float fixedWidth = numWidth + keyWidth + (showProperties ? priorityWidth + commentWidth + contextWidth : 0);
	columns.push_back(numWidth);
	columnNames.push_back("#");
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

		size_t len = 3 + (translatedData ? 1 : 0) + (showProperties ? 3 : 0);
		size_t firstIcon = len - 3;
		strs.resize(len);
		colours.resize(len, textCol);
		sprites.resize(len);

		strs[0] = toString(idx + 1);
		strs[1] = entry.key;
		strs[2] = entry.value;

		if (translatedData) {
			if (auto* translatedEntry = translatedData->tryGetEntry(entry.key)) {
				strs[3] = translatedEntry->value;
				colours[3] = entry.version == translatedEntry->origVersion ? textCol : outdatedCol;
			}
		}

		if (showProperties) {
			sprites[firstIcon] = priorityIcons.at(entry.priority);
			if (!entry.comment.isEmpty()) {
				sprites[firstIcon + 1] = commentIcon;
			}
			if (!entry.context.isEmpty()) {
				sprites[firstIcon + 2] = contextIcon;
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
			sendToClipboard(entry.key);
		} else if (choice == "copyOriginal") {
			sendToClipboard(entry.value);
		} else if (choice == "copyTranslation") {
			if (auto* translated = translatedData ? translatedData->tryGetEntry(entry.key) : nullptr) {
				sendToClipboard(translated->value);
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
		result[entry.key] = entry.value;
		keyOrder += entry.key;
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

const String& LocalisationGrid::getKeyAt(int idx) const
{
	if (origData && idx >= 0 && idx < static_cast<int>(getSrcRowCount())) {
		return origData->getEntry(idx).key;
	} else {
		return String::emptyString();
	}
}

void LocalisationGrid::setData(const ILocOriginalData* origData, LocTranslationData* translatedData, bool showProperties)
{
	this->origData = origData;
	this->translatedData = translatedData;
	this->showProperties = showProperties;

	onDataUpdated();
}

LocalisedString LocalisationGrid::getToolTip() const
{
	if (columnUnderMouse && lineUnderMouse) {
		const auto& entry = origData->getEntry(*lineUnderMouse);
		const auto colName = columnNames[*columnUnderMouse];
		if (colName == "Key") {
			String tooltip = entry.key;

			if (translatedData) {
				if (const auto* translatedEntry = translatedData->tryGetEntry(entry.key)) {
					tooltip += "\nv" + toString(translatedEntry->origVersion);
					if (translatedEntry->origVersion != entry.version) {
						tooltip += " (src at v" + toString(entry.version) + ")";
					}
				}
			} else {
				tooltip += "\nv" + toString(entry.version);
			}

			return LocalisedString::fromUserString(tooltip);
		} else if (colName == "Original") {
			return LocalisedString::fromUserString(entry.value);
		} else if (colName == "Translated") {
			if (const auto* translatedEntry = translatedData->tryGetEntry(entry.key)) {
				return LocalisedString::fromUserString(translatedEntry->value);
			}
		} else if (colName == "Pri") {
			return LocalisedString::fromUserString("Priority: " + toString(entry.priority));
		} else if (colName == "Cmt") {
			return LocalisedString::fromUserString(entry.comment);
		} else if (colName == "Ctx") {
			return LocalisedString::fromUserString(entry.context);
		}
	}
	return {};
}

bool LocalisationGrid::hasDynamicToolTip() const
{
	return true;
}

Vector2f LocalisationGrid::getToolTipPosition(Vector2f mousePos) const
{
	if (columnUnderMouse && lineUnderMouse) {
		if (const auto row = getRowForLine(*lineUnderMouse)) {
			return getCellBasePos(*row, *columnUnderMouse) + Vector2f(0, getLineHeight());
		}
	}
	return mousePos;
}
