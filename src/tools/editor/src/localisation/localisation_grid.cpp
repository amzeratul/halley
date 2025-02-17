#include "localisation_grid.h"

using namespace Halley;

LocalisationGrid::LocalisationGrid(UIFactory& factory)
	: UIGrid("localisation_grid", factory)
{
	outdatedCol = factory.getColourScheme()->getColour("ui_logWarningText");
}

size_t LocalisationGrid::getNumRows() const
{
	return origData ? origData->getNumEntries() : 0;
}

std::pair<Vector<float>, Vector<String>> LocalisationGrid::getColumns() const
{
	Vector<float> columns;
	Vector<String> columnNames;

	const float width = getSize().x - 1;
	const float numWidth = 40;
	const float keyWidth = 250;
	const float priorityWidth = 30;
	const float commentWidth = 30;
	const float contextWidth = 30;
	const float fixedWidth = numWidth + keyWidth + priorityWidth + commentWidth + contextWidth;
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

	columns.push_back(priorityWidth);
	columnNames.push_back("Pri");
	columns.push_back(commentWidth);
	columnNames.push_back("Com");
	columns.push_back(contextWidth);
	columnNames.push_back("Ctx");

	return { columns, columnNames };
}

void LocalisationGrid::getLineDrawData(int idx, Vector<String>& strs, Vector<Colour4f>& colours) const
{
	if (origData) {
		const auto& entry = origData->getEntry(idx);
		strs.push_back(toString(idx + 1));
		strs.push_back(entry.key);
		strs.push_back(entry.value);
		colours.resize(3, textCol);

		if (translatedData) {
			if (auto* translatedEntry = translatedData->tryGetEntry(entry.key)) {
				strs.push_back(translatedEntry->value);
				colours.push_back(entry.version == translatedEntry->origVersion ? textCol : outdatedCol);
			}
		}
	}
}

const String& LocalisationGrid::getKeyAt(int idx) const
{
	if (origData && idx >= 0 && idx < static_cast<int>(getNumRows())) {
		return origData->getEntry(idx).key;
	} else {
		return String::emptyString();
	}
}

void LocalisationGrid::setData(const ILocOriginalData* origData, LocTranslationData* translatedData)
{
	this->origData = origData;
	this->translatedData = translatedData;

	onDataUpdated();
}
