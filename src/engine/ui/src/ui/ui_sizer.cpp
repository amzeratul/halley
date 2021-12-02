#include "ui_sizer.h"
#include "ui_widget.h"

using namespace Halley;

UISizerEntry::UISizerEntry()
{
}

UISizerEntry::UISizerEntry(UIElementPtr widget, float proportion, Vector4f border, int fillFlags, Vector2f position)
	: element(widget)
	, proportion(proportion)
	, fillFlags(fillFlags)
	, border(border)
	, position(position)
{
	updateEnabled();
}

float UISizerEntry::getProportion() const
{
	return proportion;
}

Vector2f UISizerEntry::getMinimumSize() const
{
	return element ? element->getLayoutMinimumSize(false) : Vector2f();
}

void UISizerEntry::placeInside(Rect4f rect, Rect4f origRect, Vector2f minSize, IUIElement::IUIElementListener* listener, UISizer& sizer)
{
	Vector2f cellSize = rect.getSize();
	Vector2f anchoring;
	Vector2f size = minSize;

	if (fillFlags & UISizerAlignFlags::Top) {
		anchoring.y = 0.0f;
	}
	if (fillFlags & UISizerAlignFlags::CentreVertical) {
		anchoring.y = 0.5f;
	}
	if (fillFlags & UISizerAlignFlags::Bottom) {
		anchoring.y = 1.0f;
	}
	if (fillFlags & UISizerAlignFlags::Left) {
		anchoring.x = 0.0f;
	}
	if (fillFlags & UISizerAlignFlags::CentreHorizontal) {
		anchoring.x = 0.5f;
	}
	if (fillFlags & UISizerAlignFlags::Right) {
		anchoring.x = 1.0f;
	}
	if (fillFlags & UISizerFillFlags::FillHorizontal) {
		size.x = std::max(minSize.x, cellSize.x);
	}
	if (fillFlags & UISizerFillFlags::FillVertical) {
		size.y = std::max(minSize.y, cellSize.y);
	}

	const Vector2f spareSize = cellSize - size;
	const Vector2f pos = (rect.getTopLeft() + spareSize * anchoring).round();
	const auto finalRect = Rect4f(pos, pos + size);

	if (listener) {
		listener->onPlaceInside(finalRect, origRect, element, sizer);
	}

	if (element) {
		element->setRect(finalRect, listener);
	}
}

UIElementPtr UISizerEntry::getPointer() const
{
	return element;
}

bool UISizerEntry::isEnabled() const
{
	return enabled;
}

void UISizerEntry::updateEnabled() const
{
	enabled = !element || element->isActive();
}

void UISizerEntry::setBorder(const Vector4f& b)
{
	border = b;
}

void UISizerEntry::setProportion(float prop)
{
	proportion = prop;
}

void UISizerEntry::setPosition(Vector2f pos)
{
	position = pos;
}

Vector4f UISizerEntry::getBorder() const
{
	return border;
}

int UISizerEntry::getFillFlags() const
{
	return fillFlags;
}

Vector2f UISizerEntry::getPosition() const
{
	return position;
}

UISizer::UISizer(UISizerType type, float gap, int nColumns)
	: type(type)
	, gap(gap)
{
	if (type == UISizerType::Grid) {
		gridProportions = std::make_unique<GridProportions>();
		gridProportions->nColumns = nColumns;
	}
}

UISizer::UISizer(UISizer&& other) noexcept
{
	*this = std::move(other);
}

UISizer& UISizer::operator=(UISizer&& other) noexcept
{
	type = other.type;
	gap = other.gap;
	gridProportions = std::move(other.gridProportions);

	entries = std::move(other.entries);

	curParent = other.curParent;

	return *this;
}

Vector2f UISizer::getLayoutMinimumSize(bool force) const
{
	return computeMinimumSize(true);
}

Vector2f UISizer::computeMinimumSize(bool includeProportional) const
{
	updateEnabled();
	if (type == UISizerType::Horizontal || type == UISizerType::Vertical) {
		return computeMinimumSizeBox(includeProportional);
	} else if (type == UISizerType::Free) {
		return computeMinimumSizeBoxFree();
	} else {
		return computeMinimumSizeGrid();
	}
}

void UISizer::setRect(Rect4f rect, IUIElementListener* listener)
{
	if (type == UISizerType::Horizontal || type == UISizerType::Vertical) {
		setRectBox(rect, listener);
	} else if (type == UISizerType::Free) {
		setRectFree(rect, listener);
	} else {
		setRectGrid(rect, listener);
	}
}

void UISizer::add(std::shared_ptr<IUIElement> element, float proportion, Vector4f border, int fillFlags, Vector2f position, size_t insertPos)
{
	entries.emplace(entries.begin() + std::min(entries.size(), insertPos), UISizerEntry(element, proportion, border, fillFlags, position));
	reparentEntry(entries.back());
}

void UISizer::addSpacer(float size)
{
	auto size2d = Vector2f(type == UISizerType::Horizontal ? size : 0.0f, type == UISizerType::Vertical ? size : 0.0f);
	add(std::make_shared<UISizerSpacer>(size2d));
}

void UISizer::addStretchSpacer(float proportion)
{
	add(std::make_shared<UISizerSpacer>(), proportion);
}

void UISizer::remove(IUIElement& element)
{
	entries.erase(std::remove_if(entries.begin(), entries.end(), [&] (const UISizerEntry& e) { return e.getPointer().get() == &element; }), entries.end());
}

void UISizer::reparent(UIParent& parent)
{
	if (curParent != &parent) {
		if (curParent) {
			for (auto& e: entries) {
				unparentEntry(e);
			}
		}

		curParent = &parent;
		for (auto& e: entries) {
			reparentEntry(e);
		}
	}
}

void UISizer::reparentEntry(UISizerEntry& entry)
{
	if (curParent != nullptr) {
		auto widget = std::dynamic_pointer_cast<UIWidget>(entry.getPointer());
		if (widget) {
			curParent->addChild(widget);
		} else {
			auto sizer = std::dynamic_pointer_cast<UISizer>(entry.getPointer());
			if (sizer) {
				sizer->reparent(*curParent);
			}
		}
	}
}

void UISizer::unparentEntry(UISizerEntry& entry)
{
	if (curParent != nullptr) {
		auto widget = std::dynamic_pointer_cast<UIWidget>(entry.getPointer());
		if (widget) {
			curParent->removeChild(*widget);
		}
	}
}

UISizerType UISizer::getType() const
{
	return type;
}

size_t UISizer::size() const
{
	return entries.size();
}

const UISizerEntry& UISizer::operator[](size_t n) const
{
	return entries[n];
}

UISizerEntry& UISizer::operator[](size_t n)
{
	return entries[n];
}

void UISizer::updateEnabled() const
{
	for (auto& e: entries) {
		e.updateEnabled();
	}
}

void UISizer::swapItems(int idxA, int idxB)
{
	std::swap(entries[idxA], entries[idxB]);
}

void UISizer::clear()
{
	for (auto& e: entries) {
		auto widget = std::dynamic_pointer_cast<UIWidget>(e.getPointer());
		if (widget) {
			widget->destroy();
		} else {
			auto sizer = std::dynamic_pointer_cast<UISizer>(e.getPointer());
			if (sizer) {
				sizer->clear();
			}
		}
	}
	entries.clear();
}

bool UISizer::isActive() const
{
	for (auto& e: entries) {
		if (e.isEnabled()) {
			return true;
		}
	}
	return false;
}

void UISizer::setColumnProportions(const std::vector<float>& values)
{
	if (gridProportions) {
		gridProportions->columnProportions = values;
	}
}

void UISizer::setEvenColumns()
{
	if (gridProportions) {
		gridProportions->columnProportions.resize(gridProportions->nColumns);
		for (auto& c: gridProportions->columnProportions) {
			c = 1.0f;
		}
	}
}


void UISizer::setRowProportions(const std::vector<float>& values)
{
	if (gridProportions) {
		gridProportions->rowProportions = values;
	}
}

Vector2f UISizer::computeMinimumSizeBox(bool includeProportional) const
{
	float totalProportion = 0;

	for (auto& e: entries) {
		if (e.isEnabled()) {
			totalProportion += e.getProportion();
		}
	}

	const size_t mainAxis = type == UISizerType::Vertical ? 1 : 0;
	const size_t otherAxis = 1 - mainAxis;
	
	float main = 0;
	float biggestProportional = 0;
	float other = 0;
	
	bool first = true;
	for (auto& e: entries) {
		if (!e.isEnabled()) {
			continue;
		}

		Vector2f sz = e.getMinimumSize();
		auto border = e.getBorder();
		if (!first) {
			border[mainAxis] += gap;
		}
		first = false;
		
		other = std::max(other, sz[otherAxis] + border[otherAxis] + border[otherAxis + 2]);

		float p = e.getProportion();
		if (p > 0.0001f) {
			float s = sz[mainAxis] / p;
			biggestProportional = std::max(biggestProportional, s);
		} else {
			main += sz[mainAxis];
		}

		main += border[mainAxis] + border[mainAxis + 2];
	}
	if (includeProportional) {
		main += biggestProportional * totalProportion;
	}

	Vector2f result;
	result[mainAxis] = std::max(0.0f, main);
	result[otherAxis] = std::max(0.0f, other);

	return result;
}


void UISizer::setRectBox(Rect4f rect, IUIElementListener* listener)
{
	Vector2f pos = rect.getTopLeft();

	float totalProportion = 0;
	for (auto& e: entries) {
		if (e.isEnabled()) {
			totalProportion += e.getProportion();
		}
	}

	int mainAxis = type == UISizerType::Horizontal ? 0 : 1;
	int otherAxis = 1 - mainAxis;

	Vector2f sizerMinSize = computeMinimumSizeBox(false);
	float spare = (rect.getSize() - sizerMinSize)[mainAxis];
	
	bool first = true;
	for (auto& e: entries) {
		if (!e.isEnabled()) {
			continue;
		}

		float p = e.getProportion();
		const auto border = e.getBorder();
		Vector2f gapOffset;
		if (!first) {
			gapOffset[mainAxis] += gap;
		}
		first = false;

		Vector2f minSize = e.getMinimumSize();
		Vector2f cellSize = minSize;
		if (p > 0.0001f) {
			float propSize = std::floor(spare * p / totalProportion);
			spare -= propSize;
			totalProportion -= p;
			cellSize[mainAxis] = propSize;
		}
		cellSize[otherAxis] = rect.getSize()[otherAxis] - border[otherAxis] - border[otherAxis + 2];

		Vector2f curPos = pos + border.xy() + gapOffset;
		const auto rect = Rect4f(curPos, curPos + cellSize);
		const auto origRect = Rect4f(rect.getTopLeft() - border.xy(), rect.getBottomRight() + border.zw());
		e.placeInside(rect, origRect, minSize, listener, *this);

		pos[mainAxis] += cellSize[mainAxis] + border[mainAxis] + border[mainAxis + 2] + gapOffset[mainAxis];
	}
}

Vector2f UISizer::computeMinimumSizeBoxFree() const
{
	Rect4f rect = Rect4f(0, 0, 1, 1); // Always include origin

	for (const auto& e : entries) {
		if (!e.isEnabled()) {
			continue;
		}

		const auto size = e.getMinimumSize();
		const auto position = e.getPosition();
		auto entryRect = Rect4f(position.x, position.y, size.x, size.y);
		rect = rect.merge(entryRect);
	}

	return rect.getSize();
}

void UISizer::setRectFree(Rect4f rect, IUIElementListener* listener)
{
	const auto startPos = rect.getTopLeft();

	for (auto& e : entries) {
		if (!e.isEnabled()) {
			continue;
		}
		
		const auto minSize = e.getMinimumSize();
		const auto cellSize = rect.getSize() - e.getPosition();
		const auto curPos = startPos + e.getPosition();
		const auto rect = Rect4f(curPos, curPos + cellSize);
		e.placeInside(rect, rect, minSize, listener, *this);
	}
}

void UISizer::computeGridSizes(std::vector<float>& colSize, std::vector<float>& rowSize) const
{
	Expects(gridProportions);
	auto& nColumns = gridProportions->nColumns;

	Expects(nColumns > 0);
	int nRows = std::max(1, int((entries.size() + nColumns - 1) / nColumns));

	colSize.resize(nColumns, 0.0f);
	rowSize.resize(nRows, 0.0f);
	
	// Update the minimum requirement for each cell
	{
		int i = 0;
		for (auto& e: entries) {
			int x = i % nColumns;
			int y = i / nColumns;
			++i;

			if (!e.isEnabled()) {
				continue;
			}

			Vector2f sz = e.getMinimumSize();
			auto border = e.getBorder();
			colSize[x] = std::max(colSize[x], sz.x + border.x + border.z);
			rowSize[y] = std::max(rowSize[y], sz.y + border.y + border.w);
		}
	}

	// From here on: ensure that columns respect the proportion between them
	Vector2f minMult;

	// First pass: gather data
	for (int i = 0; i < int(colSize.size()); ++i) {
		float p = getColumnProportion(i);
		if (p > 0) {
			minMult.x = std::max(minMult.x, colSize[i] / p);
		}
	}
	for (int i = 0; i < int(rowSize.size()); ++i) {
		float p = getRowProportion(i);
		if (p > 0) {
			minMult.y = std::max(minMult.y, rowSize[i] / p);
		}
	}

	// Second pass: apply
	for (int i = 0; i < int(colSize.size()); ++i) {
		float p = getColumnProportion(i);
		if (p > 0) {
			colSize[i] = std::max(colSize[i], p * minMult.x);
		}
	}
	for (int i = 0; i < int(rowSize.size()); ++i) {
		float p = getRowProportion(i);
		if (p > 0) {
			rowSize[i] = std::max(rowSize[i], p * minMult.y);
		}
	}
}

Vector2f UISizer::computeMinimumSizeGrid() const
{
	std::vector<float> colSize;
	std::vector<float> rowSize;
	computeGridSizes(colSize, rowSize);

	Vector2f result((colSize.size() - 1) * gap, (rowSize.size() - 1) * gap);
	for (auto c: colSize) {
		result.x += c;
	}
	for (auto c: rowSize) {
		result.y += c;
	}
	return result;
}

void UISizer::setRectGrid(Rect4f rect, IUIElementListener* listener)
{
	Expects(gridProportions);
	auto& nColumns = gridProportions->nColumns;
	auto& columnProportions = gridProportions->columnProportions;
	auto& rowProportions = gridProportions->rowProportions;

	Expects(nColumns > 0);
	int nRows = int((entries.size() + nColumns - 1) / nColumns);
	
	Vector2f startPos = rect.getTopLeft();

	std::vector<float> cols(nColumns);
	std::vector<float> rows(nRows);
	std::vector<float> colSize;
	std::vector<float> rowSize;
	computeGridSizes(colSize, rowSize);

	// Add up min width
	float minWidth = (colSize.size() - 1) * gap;
	float minHeight = (rowSize.size() - 1) * gap;
	for (auto c: colSize) {
		minWidth += c;
	}
	for (auto r: rowSize) {
		minHeight += r;
	}
	float spareWidth = std::max(0.0f, rect.getWidth() - minWidth);
	float spareHeight = std::max(0.0f, rect.getHeight() - minHeight);

	// Respect proportion
	float totalColProportion = 0;
	for (auto& p: columnProportions) {
		totalColProportion += p;
	}
	if (totalColProportion > 0) {
		// Distribute spare
		for (int i = 0; i < nColumns; ++i) {
			colSize[i] += spareWidth * getColumnProportion(i) / totalColProportion;
		}
	}
	float totalRowProportion = 0;
	for (auto& p: rowProportions) {
		totalRowProportion += p;
	}
	if (totalRowProportion > 0) {
		// Distribute spare
		for (int i = 0; i < nRows; ++i) {
			rowSize[i] += spareHeight * getRowProportion(i) / totalRowProportion;
		}
	}

	{
		float x = 0;
		for (int i = 0; i < nColumns; ++i) {
			cols[i] = x;
			x += colSize[i] + gap;
		}
		float y = 0;
		for (int i = 0; i < nRows; ++i) {
			rows[i] = y;
			y += rowSize[i] + gap;
		}
	}

	int i = 0;
	for (auto& e: entries) {
		int x = i % nColumns;
		int y = i / nColumns;
		++i;

		if (!e.isEnabled()) {
			continue;
		}

		Vector2f cellSize(colSize[x], rowSize[y]);
		Vector2f sz = e.getMinimumSize();
		auto border = e.getBorder();
		Vector2f curPos = Vector2f(cols[x], rows[y]) + startPos + Vector2f(border.x, border.y);
		e.placeInside(Rect4f(curPos, curPos + cellSize), Rect4f(startPos, startPos + cellSize), sz, listener, *this);
	}
}

float UISizer::getColumnProportion(int col) const
{
	Expects(gridProportions);
	auto& columnProportions = gridProportions->columnProportions;

	if (col >= 0 && col < int(columnProportions.size())) {
		return columnProportions[col];
	}
	return 0.0f;
}

float UISizer::getRowProportion(int row) const
{
	Expects(gridProportions);
	auto& rowProportions = gridProportions->rowProportions;

	if (row >= 0 && row < int(rowProportions.size())) {
		return rowProportions[row];
	}
	return 0.0f;
}

void UISizer::sortChildrenBySizerOrder()
{
	auto& children = curParent->getChildren();
	if (children.size() == entries.size()) {
		for (size_t i = 0; i < children.size(); ++i) {
			children[i] = std::dynamic_pointer_cast<UIWidget>(entries[i].getPointer());
		}
	}
}
