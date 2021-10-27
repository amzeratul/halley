#include "widgets/ui_list.h"
#include "ui_style.h"
#include "halley/core/input/input_keyboard.h"
#include "widgets/ui_label.h"
#include "widgets/ui_image.h"
#include "halley/support/logger.h"

using namespace Halley;

UIList::UIList(String id, UIStyle style, UISizerType orientation, int nColumns)
	: UIWidget(std::move(id), {}, UISizer(orientation, style.getFloat("gap"), nColumns), style.getBorder("innerBorder"))
	, orientation(orientation)
	, nColumns(nColumns)
{
	styles.emplace_back(style);
	getSizer().setEvenColumns();
	sprite = style.getSprite("background");

	setHandle(UIEventType::SetSelected, [=] (const UIEvent& event) {});
	setHandle(UIEventType::SetHovered, [=] (const UIEvent& event) {
		const auto hoveredChild = std::find_if(getChildren().begin(), getChildren().end(), [=](std::shared_ptr<UIWidget> child) { return child->getId() == event.getSourceId(); });
		const auto childIdx = int(hoveredChild - getChildren().begin());

		if (event.getBoolData()) {
			curHover = childIdx;
			sendEvent(UIEvent(UIEventType::ListHoveredChanged, getId(), event.getSourceId(), curHover));
		} else if (curHover == childIdx) {
			curHover = -1;
			sendEvent(UIEvent(UIEventType::ListHoveredChanged, getId(), "", -1));
		}
	});
}

void UIList::setOrientation(UISizerType orientation, int nColumns)
{
	Expects(items.empty());
	
	if (orientation != this->orientation || nColumns != this->nColumns) {
		setSizer(UISizer(orientation, styles.at(0).getFloat("gap"), nColumns));
		getSizer().setEvenColumns();
		this->orientation = orientation;
		this->nColumns = nColumns;
	}
}

UISizerType UIList::getOrientation() const
{
	return orientation;
}

bool UIList::setSelectedOption(int option, SelectionMode mode)
{
	if (!multiSelect) {
		mode = SelectionMode::Normal;
	}
	
	forceAddChildren(UIInputType::Undefined, false);

	const auto numberOfItems = int(getNumberOfItems());
	if (numberOfItems == 0) {
		return false;
	}
	const auto newSel = clamp(option, 0, numberOfItems - 1);

	if (!requiresSelection && option < 0) {
		deselectAll();
		curOption = -1;
		return false;
	}
	
	if (newSel != curOption) {
		changeSelection(curOption, newSel, mode);

		if (mode != SelectionMode::ShiftSelect) {
			const auto curItem = getItem(newSel);
			if (curItem->isEnabled()) {
				notifyNewItemSelected(newSel, curItem->getId());
				curOption = newSel;
				return true;
			}
		}
	}
	return false;
}

int UIList::getSelectedOption() const
{
	return curOption;
}

String UIList::getSelectedOptionId() const
{
	if (curOption < 0 || curOption >= int(getNumberOfItems())) {
		return "";
	}
	return getItem(curOption)->getId();
}

size_t UIList::getCount() const
{
	return getNumberOfItems();
}

UIStyle UIList::getStyle() const
{
	return styles.at(0);
}

std::shared_ptr<UILabel> UIList::makeLabel(String id, LocalisedString label, float maxWidth) const
{
	const auto& style = styles.at(0);
	auto widget = std::make_shared<UILabel>(std::move(id), style, std::move(label));
	if (maxWidth > 0) {
		widget->setMaxWidth(maxWidth);
	}

	if (style.hasTextRenderer("selectedLabel")) {
		widget->setSelectable(style.getTextRenderer("label"), style.getTextRenderer("selectedLabel"));
	}

	if (style.hasTextRenderer("disabledLabel")) {
		widget->setDisablable(style.getTextRenderer("label"), style.getTextRenderer("disabledLabel"));
	}

	return widget;
}

std::shared_ptr<UIImage> UIList::makeIcon(Sprite image) const
{
	auto icon = std::make_shared<UIImage>(image);
	applyImageColour(*icon);	
	return icon;
}

Vector2f UIList::getDragPositionAdjustment(Vector2f pos, Vector2f startPos) const
{
	if (orientation == UISizerType::Horizontal) {
		pos.y = startPos.y;
	} else if (orientation == UISizerType::Vertical) {
		pos.x = startPos.x;
	}
	return pos;
}

std::shared_ptr<UIListItem> UIList::addTextItem(const String& id, LocalisedString label, float maxWidth, bool centre, std::optional<LocalisedString> tooltip)
{
	// Use addTextItemAlignedInstead
	return addTextItemAligned(id, std::move(label), maxWidth, {}, centre ? UISizerAlignFlags::CentreHorizontal : UISizerFillFlags::Fill, std::move(tooltip));
}

std::shared_ptr<UIListItem> UIList::addTextItemAligned(const String& id, LocalisedString label, float maxWidth, Vector4f border, int fillFlags, std::optional<LocalisedString> tooltip)
{
	const auto& style = styles.at(0);
	auto item = std::make_shared<UIListItem>(id, *this, style.getSubStyle("item"), int(getNumberOfItems()), style.getBorder("extraMouseBorder"));
	if (tooltip) {
		item->setToolTip(tooltip.value());
	}
	item->add(makeLabel(id + "_label", std::move(label), maxWidth), 0, border, fillFlags);
	return addItem(item);
}

std::shared_ptr<UIListItem> UIList::addTextIconItem(const String& id, LocalisedString label, Sprite icon, float maxWidth, Vector4f border, int fillFlags, std::optional<LocalisedString> tooltip)
{
	const auto& style = styles.at(0);
	auto item = std::make_shared<UIListItem>(id, *this, style.getSubStyle("item"), int(getNumberOfItems()), style.getBorder("extraMouseBorder"));
	if (tooltip) {
		item->setToolTip(tooltip.value());
	}

	const bool hasIcon = icon.hasMaterial();
	const bool hasLabel = !label.getString().isEmpty();
	
	if (hasIcon) {
		item->add(makeIcon(icon), 0, hasLabel ? Vector4f(0, 0, 4, 0) : border, hasLabel ? UISizerAlignFlags::Centre : fillFlags);
	}
	if (hasLabel) {
		item->add(makeLabel(id + "_label", std::move(label), maxWidth), 0, border, fillFlags);
	}
	
	return addItem(item);
}

std::shared_ptr<UIListItem> UIList::addImage(const String& id, std::shared_ptr<UIImage> image, float proportion, Vector4f border, int fillFlags, std::optional<UIStyle> styleOverride)
{
	applyImageColour(*image);
	return addItem(id, image, proportion, border, fillFlags, std::move(styleOverride));
}

void UIList::applyImageColour(UIImage& image) const
{
	const auto& style = styles.at(0);
	Colour4f baseCol;
	if (style.hasColour("imageColour")) {
		baseCol = style.getColour("imageColour");
		image.getSprite().setColour(baseCol);
	} else {
		baseCol = image.getSprite().getColour();
	}
	if (style.hasColour("selectedImageColour")) {
		image.setSelectable(baseCol, style.getColour("selectedImageColour"));
	}
	if (style.hasColour("hoveredImageColour")) {
		image.setHoverable(baseCol, style.getColour("hoveredImageColour"));
	}
	if (style.hasColour("disabledImageColour")) {
		image.setDisablable(baseCol, style.getColour("disabledImageColour"));
	}
}

void UIList::changeSelection(int oldItem, int newItem, SelectionMode mode)
{
	if (mode == SelectionMode::Normal || mode == SelectionMode::ShiftSelect) {
		deselectAll();
	}

	if (mode == SelectionMode::Normal || mode == SelectionMode::CtrlSelect) {
		const auto curItem = tryGetItem(newItem);
		if (curItem->isEnabled()) {
			curItem->setSelected(mode == SelectionMode::Normal ? true : !curItem->isSelected());
		}
	} else if (mode == SelectionMode::ShiftSelect) {
		const int a = std::min(oldItem, newItem);
		const int b = std::max(oldItem, newItem);
		for (int i = a; i <= b; ++i) {
			const auto curItem = tryGetItem(i);
			if (curItem->isEnabled()) {
				curItem->setSelected(true);
			}
		}
	}
}

void UIList::deselectAll()
{
	for (auto& item: items) {
		item->setSelected(false);
	}
}

void UIList::notifyNewItemSelected(int itemIdx, const String& itemId)
{
	playSound(styles.at(0).getString("selectionChangedSound"));

	sendEvent(UIEvent(UIEventType::ListSelectionChanged, getId(), itemId, itemIdx));
	if (scrollToSelection) {
		sendEvent(UIEvent(UIEventType::MakeAreaVisible, getId(), getOptionRect(itemIdx)));
	}
	
	if (getDataBindFormat() == UIDataBind::Format::String) {
		notifyDataBind(itemId);
	} else {
		notifyDataBind(itemIdx);
	}
}

std::shared_ptr<UIListItem> UIList::addItem(const String& id, std::shared_ptr<IUIElement> element, float proportion, Vector4f border, int fillFlags, std::optional<UIStyle> styleOverride)
{
	const auto& itemStyle = styleOverride ? *styleOverride : styles.at(0);
	auto item = std::make_shared<UIListItem>(id, *this, itemStyle.getSubStyle("item"), int(getNumberOfItems()), itemStyle.getBorder("extraMouseBorder"));
	item->add(element, proportion, border, fillFlags);
	return addItem(item, Vector4f{}, fillFlags);
}

void UIList::clear()
{
	items.clear();
	curOption = -1;
	UIWidget::clear();
	layout();

	if (scrollToSelection) {
		sendEvent(UIEvent(UIEventType::MakeAreaVisibleCentered, getId(), Rect4f(0, 0, 1, 1)));
	}
}

void UIList::setItemEnabled(const String& id, bool enabled)
{
	auto curId = getSelectedOptionId();
	for (auto& item: items) {
		if (item->getId() == id) {
			if (!enabled) {
				item->setSelected(false);
			}
			item->setEnabled(enabled);
		}
	}
	reassignIds();

	resetSelectionIfInvalid();
}

void UIList::resetSelectionIfInvalid()
{
	if (!setSelectedOptionId(getSelectedOptionId())) {
		const auto shouldResetSelection = (requiresSelection || curOption >= 0);
		curOption = -1;
		if (shouldResetSelection) {
			setSelectedOption(0);
		}
	}
}

void UIList::setItemActive(const String& id, bool active)
{
	doSetItemActive(id, active);
	reassignIds();
	resetSelectionIfInvalid();
}

void UIList::doSetItemActive(const String& id, bool active)
{
	for (auto& item: items) {
		if (item->getId() == id) {
			if (!active) {
				item->setSelected(false);
			}
			item->setActive(active);
		}
	}
}

void UIList::filterOptions(const String& filter)
{
	auto filterLower = filter.asciiLower();
	
	const auto curId = getSelectedOptionId();
	for (auto& item: items) {
		const bool active = item->getId().asciiLower().contains(filterLower);
		if (!active) {
			item->setSelected(false);
		}
		item->setActive(active);
	}

	layout();
	reassignIds();
	resetSelectionIfInvalid();
}

std::shared_ptr<UIListItem> UIList::addItem(std::shared_ptr<UIListItem> item, Vector4f border, int fillFlags)
{
	add(item, uniformSizedItems ? 1.0f : 0.0f, border, fillFlags);
	items.push_back(item);

	if (getNumberOfItems() == 1 && requiresSelection) {
		curOption = -1;
		setSelectedOption(0);
	}

	return items.back();
}

std::optional<int> UIList::removeItem(const String& id)
{
	const auto iter = std::find_if(items.begin(), items.end(), [&] (const std::shared_ptr<UIListItem>& item)
	{
		return item->getId() == id;
	});
	if (iter != items.end()) {
		const auto idx = static_cast<int>(iter - items.begin());
		removeItem(idx);
		return idx;
	}

	return {};
}

void UIList::removeItem(int idx)
{
	if (idx >= 0 && idx < static_cast<int>(items.size())) {
		const auto item = items[idx];
		remove(*item);
		items.erase(items.begin() + idx);

		layout();
		reassignIds();

		if (curOption >= idx) {
			setSelectedOption(curOption - 1);
		}
	}
}

void UIList::draw(UIPainter& painter) const
{
	if (sprite.hasMaterial()) {
		painter.draw(sprite);
	}
}


void UIList::onAccept()
{
	sendEvent(UIEvent(UIEventType::ListAccept, getId(), getSelectedOptionId(), curOption));
}

void UIList::onCancel()
{
	sendEvent(UIEvent(UIEventType::ListCancel, getId(), getSelectedOptionId(), curOption));
}

void UIList::reassignIds()
{
	int i = 0;
	int j = 0;
	for (auto& item: items) {
		if (item->isActive() && item->isEnabled()) {
			item->setIndex(i++);
		} else {
			item->setIndex(-1);
		}
		item->setAbsoluteIndex(j++);
	}
}

std::shared_ptr<UIListItem> UIList::getItem(int n) const
{
	if (n < 0) {
		throw Exception("Invalid item", HalleyExceptions::UI);
	}
	int i = 0;
	for (auto& item: items) {
		if (item->isActive() && item->isEnabled()) {
			if (i++ == n) {
				return item;
			}
		}
	}
	throw Exception("Invalid item", HalleyExceptions::UI);
}

std::shared_ptr<UIListItem> UIList::getItem(const String& id) const
{
	for (auto& item: items) {
		if (item->getId() == id) {
			return item;
		}
	}
	throw Exception("Invalid item", HalleyExceptions::UI);
}

int UIList::tryGetItemId(const String& id) const
{
	const auto iter = std::find_if(items.begin(), items.end(), [&] (const std::shared_ptr<UIListItem>& item)
	{
		return item->getId() == id;
	});
	if (iter != items.end()) {
		const size_t idx = iter - items.begin();
		return static_cast<int>(idx);
	}
	return -1;
}

std::shared_ptr<UIListItem> UIList::tryGetItem(int n) const
{
	int i = 0;
	for (auto& item: items) {
		if (item->isActive() && item->isEnabled()) {
			if (i++ == n) {
				return item;
			}
		}
	}
	return {};
}

std::shared_ptr<UIListItem> UIList::tryGetItem(const String& id) const
{
	for (auto& item : items) {
		if (item->getId() == id) {
			return item;
		}
	}
	return {};
}

std::shared_ptr<UIListItem> UIList::getItemUnderCursor() const
{
	if (itemUnderCursor >= 0 && itemUnderCursor < static_cast<int>(items.size())) {
		return items[itemUnderCursor];
	} else {
		return {};
	}
}

void UIList::changeItemId(int idx, const String& newId)
{
	items.at(idx)->setId(newId);
}

bool UIList::isDragEnabled() const
{
	return dragEnabled;
}

void UIList::setDragEnabled(bool drag)
{
	dragEnabled = drag;
}

bool UIList::isDragOutsideEnabled() const
{
	return dragOutsideEnabled;
}

void UIList::setDragOutsideEnabled(bool dragOutside)
{
	dragOutsideEnabled = dragOutside;
}

bool UIList::canDragListItem(const UIListItem& listItem)
{
	return isDragEnabled();
}

void UIList::setUniformSizedItems(bool enabled)
{
	uniformSizedItems = enabled;
}

void UIList::setScrollToSelection(bool enabled)
{
	scrollToSelection = enabled;
}

bool UIList::ignoreClip() const
{
	return true;
}

bool UIList::canReceiveFocus() const
{
	return focusable;
}

void UIList::setFocusable(bool f)
{
	focusable = f;
}

bool UIList::isMultiSelect() const
{
	return multiSelect;
}

void UIList::setMultiSelect(bool enabled)
{
	multiSelect = enabled;
}

void UIList::setRequiresSelection(bool r)
{
	requiresSelection = r;
}

size_t UIList::getNumberOfItems() const
{
	size_t n = 0;
	for (auto& item: items) {
		if (item->isActive() && item->isEnabled()) {
			++n;
		}
	}
	return n;
}

void UIList::swapItems(int idxA, int idxB)
{
	if (curOption == idxA) {
		curOption = idxB;
	} else if (curOption == idxB) {
		curOption = idxA;
	}

	std::swap(items[idxA], items[idxB]);
	reassignIds();
	getSizer().swapItems(idxA, idxB);

	const auto sizeA = items[idxA]->getSize();
	const auto sizeB = items[idxB]->getSize();
	Vector2f deltaSize;
	if (orientation == UISizerType::Horizontal) {
		deltaSize.x = sizeB.x - sizeA.x;
	} else if (orientation == UISizerType::Vertical) {
		deltaSize.y = sizeB.y - sizeA.y;
	}
	items[idxA]->notifySwap(items[idxB]->getOrigPosition() + (idxB < idxA ? deltaSize : Vector2f()));
	items[idxB]->notifySwap(items[idxA]->getOrigPosition() - (idxB > idxA ? deltaSize : Vector2f()));

	sendEvent(UIEvent(UIEventType::ListItemsSwapped, getId(), idxA, idxB));
}

bool UIList::isManualDragging() const
{
	return manualDragging;
}

void UIList::setItemUnderCursor(int itemIdx, bool isMouseOver)
{
	if (isMouseOver) {
		itemUnderCursor = itemIdx;
	} else if (itemUnderCursor == itemIdx) {
		itemUnderCursor = -1;
	}
}

void UIList::onGamepadInput(const UIInputResults& input, Time time)
{
	if (getNumberOfItems() == 0) {
		curOption = 0;
		return;
	}

	Expects(nColumns >= 1);

	// Drag
	if (dragEnabled && input.isButtonHeld(UIGamepadInput::Button::Hold)) {
		// Manual dragging
		manualDragging = true;

		int dir = 0;
		if (input.isButtonPressed(UIGamepadInput::Button::Next)) {
			dir++;
		} else if (input.isButtonPressed(UIGamepadInput::Button::Prev)) {
			dir--;
		} else if (orientation == UISizerType::Horizontal) {
			dir += input.getAxisRepeat(UIGamepadInput::Axis::X);
		} else if (orientation == UISizerType::Vertical) {
			dir += input.getAxisRepeat(UIGamepadInput::Axis::Y);
		}
		dir = clamp(dir, -1, 1);
		if (dir != 0) {
			int other = clamp(0, curOption + dir, int(getCount()) - 1);
			if (curOption != other) {
				swapItems(curOption, other);
			}
		}
	} else {
		// Not (manually) dragging!
		manualDragging = false;

		// Next/prev first
		if (input.isButtonPressed(UIGamepadInput::Button::Next)) {
			setSelectedOption(modulo(curOption + 1, static_cast<int>(getNumberOfItems())));
		}
		if (input.isButtonPressed(UIGamepadInput::Button::Prev)) {
			setSelectedOption(modulo(curOption - 1, static_cast<int>(getNumberOfItems())));
		}

		moveSelection(input.getAxisRepeat(UIGamepadInput::Axis::X), input.getAxisRepeat(UIGamepadInput::Axis::Y));
	}

	if (input.isButtonPressed(UIGamepadInput::Button::Accept)) {
		onAccept();
	}

	if (input.isButtonPressed(UIGamepadInput::Button::Cancel)) {
		onCancel();
	}
}

bool UIList::onKeyPress(KeyboardKeyPress key)
{
	if (key.is(KeyCode::Up)) {
		moveSelection(0, -1);
		return true;
	}

	if (key.is(KeyCode::Down)) {
		moveSelection(0, 1);
		return true;
	}

	if (key.is(KeyCode::Left)) {
		moveSelection(-1, 0);
		return true;
	}

	if (key.is(KeyCode::Right)) {
		moveSelection(1, 0);
		return true;
	}

	if (key.is(KeyCode::Enter)) {
		onAccept();
		return true;
	}

	return false;
}

void UIList::moveSelection(int dx, int dy)
{
	if (dx == 0 && dy == 0) {
		return;
	}
	
	int nCols;
	int nRows;
	int option = curOption;

	const auto nItems = int(getNumberOfItems());
	option = modulo(option, nItems);

	Vector2i cursorPos;
	if (orientation == UISizerType::Horizontal) {
		nRows = 1;
		nCols = nItems;
		cursorPos = Vector2i(option, 0);
	} else if (orientation == UISizerType::Vertical) {
		nRows = nItems;
		nCols = 1;
		cursorPos = Vector2i(0, option);
	} else {
		nRows = (nItems + nColumns - 1) / nColumns;
		nCols = nColumns;
		cursorPos = Vector2i(option % nCols, option / nCols);
	}

	// Arrows
	cursorPos.x += dx;
	cursorPos.y += dy;
	cursorPos.y = modulo(cursorPos.y, nRows);
	int columnsThisRow = (cursorPos.y == nRows - 1) ? nItems % nCols : nCols;
	if (columnsThisRow == 0) { // If the last column is full, this will happen
		columnsThisRow = nCols;
	}
	cursorPos.x = modulo(cursorPos.x, columnsThisRow); // The last row has fewer elements

	// Actually update the selection, if it changed
	setSelectedOption(cursorPos.x + cursorPos.y * nCols);
}

void UIList::update(Time t, bool moved)
{
	if (moved) {
		if (sprite.hasMaterial()) {
			sprite.scaleTo(getSize()).setPos(getPosition());
		}
	}

	if (firstUpdate) {
		if (scrollToSelection) {
			sendEvent(UIEvent(UIEventType::MakeAreaVisibleCentered, getId(), getOptionRect(curOption)));
		}
		firstUpdate = false;
	}
}

void UIList::onItemClicked(UIListItem& item, int button, KeyMods keyMods)
{
	if (button == 0 || !singleClickAccept) {
		const bool shiftHeld = (static_cast<int>(keyMods) & static_cast<int>(KeyMods::Shift)) != 0;
		const bool ctrlHeld = (static_cast<int>(keyMods) & static_cast<int>(KeyMods::Ctrl)) != 0;
		setSelectedOption(item.getIndex(), shiftHeld ? SelectionMode::ShiftSelect : (ctrlHeld ? SelectionMode::CtrlSelect : SelectionMode::Normal));
	}
	if (button == 0 && singleClickAccept) {
		onAccept();
	}
	if (button == 1) {
		sendEvent(UIEvent(UIEventType::ListItemMiddleClicked, getId(), item.getId(), curOption));
	} else if (button == 2) {
		sendEvent(UIEvent(UIEventType::ListItemRightClicked, getId(), item.getId(), curOption));
	}
	focus();
}

void UIList::onItemDoubleClicked(UIListItem& item)
{
	setSelectedOption(item.getIndex());
	onAccept();
}

void UIList::onItemDragging(UIListItem& item, int index, Vector2f pos)
{
	const int axis = orientation == UISizerType::Horizontal ? 0 : 1;

	if (index > 0) {
		auto& prev = items[index - 1];
		if (prev->canSwap() && pos[axis] < prev->getPosition()[axis] + 2.0f) {
			swapItems(index - 1, index);
		}
	}

	if (index < int(items.size()) - 1) {
		auto& next = items[index + 1];
		if (next->canSwap() && pos[axis] + item.getSize()[axis] > next->getPosition()[axis] + next->getSize()[axis] - 2.0f) {
			swapItems(index, index + 1);
		}
	}
}

void UIList::onItemDoneDragging(UIListItem& item, int index, Vector2f pos)
{
}

UIListItem::UIListItem(const String& id, UIList& parent, UIStyle style, int index, Vector4f extraMouseArea)
	: UIClickable(id, {}, UISizer(UISizerType::Horizontal), style.getBorder("innerBorder"))
	, parent(parent)
	, style(style)
	, index(index)
	, absoluteIndex(index)
	, extraMouseArea(extraMouseArea)
	, dragWidget(this)
{
	sprite = style.getSprite("normal");
	setMinSize(style.getVector2f("minSize", Vector2f()));
}

void UIListItem::onDoubleClicked(Vector2f mousePos, KeyMods keyMods)
{
	parent.onItemDoubleClicked(*this);
}

void UIListItem::onClicked(Vector2f mousePos, KeyMods keyMods)
{
}

void UIListItem::setSelected(bool s)
{
	if (selected != s) {
		selected = s;
		doSetState(getCurState());

		sendEventDown(UIEvent(UIEventType::SetSelected, getId(), selected));
	}
}

bool UIListItem::isSelected() const
{
	return selected;
}

void UIListItem::setStyle(UIStyle style)
{
	this->style = style;
	doSetState(getCurState());
}

void UIListItem::onEnabledChanged()
{
	if (!isEnabled() && selected) {
		setSelected(false);
	}
	addNewChildren(getLastInputType());
	doSetState(getCurState());
	sendEventDown(UIEvent(UIEventType::SetEnabled, getId(), isEnabled()));
}

void UIListItem::draw(UIPainter& painter) const
{
	if (sprite.hasMaterial()) {
		if (dragged) {
			auto p2 = painter.withAdjustedLayer(1);
			p2.draw(sprite);
		} else {
			painter.draw(sprite);
		}
	}
}

void UIListItem::update(Time t, bool moved)
{
	bool dirty = updateButton() || moved;

	origPos = getPosition();

	if (dragged) {
		setChildLayerAdjustment(1);

		auto pos = curDragPos;
		if (!parent.dragOutsideEnabled) {
			const auto parentRect = parent.getRect();
			const auto myTargetRect = Rect4f(curDragPos, curDragPos + dragWidget->getSize());
			pos = myTargetRect.fitWithin(parentRect).getTopLeft();
		}
		dragWidget->setPosition(pos);
		dragWidget->layout();
		dirty = true;
	} else  {
		setChildLayerAdjustment(0);
		if (swapping) {
			swapTime += t;
			constexpr Time totalTime = 0.15;
			if (swapTime > totalTime) {
				swapping = false;
			}
			float p = clamp(float(swapTime / totalTime), 0.0f, 1.0f);
			setPosition(lerp(swapFrom, swapTo, p));
			layout();
			dirty = true;
		}
	}

	const bool manualDragging = isManualDragging();
	if (dragged || manualDragging || manualDragTime > 0) {
		if (manualDragging) {
			manualDragTime = 1;
		} else {
			manualDragTime = 0;
		}
		doSetState(getCurState());
	}

	if (dirty) {
		updateSpritePosition();
	}
}

void UIListItem::onMouseOver(Vector2f mousePos)
{
	if (held && parent.canDragListItem(*this) && (mousePos - mouseStartPos).length() > 3.0f) {
		dragged = true;
		setNoClipChildren(parent.isDragOutsideEnabled());
	}
	if (dragged) {
		setDragPos(parent.getDragPositionAdjustment(mousePos - mouseStartPos + myStartPos, myStartPos));
	}
}

void UIListItem::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	UIClickable::pressMouse(mousePos, button, keyMods);
	if (button == 0) {
		held = true;
		dragged = false;
		mouseStartPos = mousePos;

		if (!dragWidget) {
			dragWidget = this;
		}
		myStartPos = dragWidget->getPosition();
		dragWidgetOffset = myStartPos - getPosition();
	}

	if (button == 2) {
		held = false;
		dragged = false;
	}

	parent.onItemClicked(*this, button, keyMods);
}

void UIListItem::releaseMouse(Vector2f mousePos, int button)
{
	UIClickable::releaseMouse(mousePos, button);
	if (button == 0) {
		if (held) {
			onMouseOver(mousePos);
			held = false;

			if (dragged) {
				dragged = false;
				setNoClipChildren(false);
				parent.onItemDoneDragging(*this, index, curDragPos);
			}
		}
	}
}

void UIListItem::setDragPos(Vector2f pos)
{
	curDragPos = pos.round();
	parent.onItemDragging(*this, index, curDragPos);
}

void UIListItem::doSetState(State state)
{
	if (dragged || isManualDragging()) {
		sprite = style.getSprite("drag");
	} else if (selected && style.hasSprite("selected")) {
		sprite = style.getSprite("selected");
	} else if (!isEnabled() && style.hasSprite("disabled")) {
		sprite = style.getSprite("disabled");
	} else {
		switch (state) {
		case State::Up:
			sprite = style.getSprite("normal");
			sendEventDown(UIEvent(UIEventType::SetHovered, getId(), false));
			sendEvent(UIEvent(UIEventType::SetHovered, getId(), false));
			break;
		case State::Hover:
			sprite = style.getSprite("hover");
			sendEventDown(UIEvent(UIEventType::SetHovered, getId(), true));
			sendEvent(UIEvent(UIEventType::SetHovered, getId(), true));
			break;
		case State::Down:
			sprite = style.hasSprite("selected") ? style.getSprite("selected") : style.getSprite("hover");
			break;
		}
	}
	updateSpritePosition();

	parent.setItemUnderCursor(index, isMouseOver());
}

void UIListItem::updateSpritePosition()
{
	if (sprite.hasMaterial()) {
		Vector2f pos = getPosition();

		if (dragged) {
			pos = dragWidget->getPosition() - dragWidgetOffset;
		}
		sprite.scaleTo(getSize() - innerBorder.xy() - innerBorder.zw()).setPos(pos + innerBorder.xy());
	}
}

bool UIListItem::isManualDragging() const
{
	return parent.isManualDragging() && parent.getSelectedOption() == index;
}

int UIListItem::getIndex() const
{
	return index;
}

void UIListItem::setIndex(int i)
{
	index = i;
}

int UIListItem::getAbsoluteIndex() const
{
	return absoluteIndex;
}

void UIListItem::setAbsoluteIndex(int index)
{
	absoluteIndex = index;
}

Rect4f UIListItem::getMouseRect() const
{
	auto rect = UIClickable::getMouseRect();
	if (rect.getWidth() <= 0.01f || rect.getHeight() <= 0.01f) {
		return rect;
	}
	return Rect4f(rect.getTopLeft() - extraMouseArea.xy() + innerBorder.xy(), rect.getBottomRight() + extraMouseArea.zw() - innerBorder.zw());
}

Rect4f UIListItem::getRawRect() const
{
	return Rect4f(getPosition(), getPosition() + getSize());
}

void UIListItem::setClickableInnerBorder(Vector4f ib)
{
	innerBorder = ib;
}

Vector4f UIListItem::getClickableInnerBorder() const
{
	return innerBorder;
}

void UIListItem::notifySwap(Vector2f to)
{
	if (!dragged) {
		swapping = true;
		swapTime = 0;
		swapFrom = getPosition();
		swapTo = to;
	}
}

bool UIListItem::canSwap() const
{
	return !swapping;
}

Vector2f UIListItem::getOrigPosition() const
{
	return origPos;
}

void UIListItem::setDraggableSubWidget(UIWidget* widget)
{
	dragWidget = widget;
}

bool UIList::setSelectedOptionId(const String& id, SelectionMode mode)
{
	for (auto& i: items) {
		if (i->getId() == id) {
			if (i->isActive()) {
				setSelectedOption(i->getIndex(), mode);
				return true;
			}
		}
	}
	return false;
}

Rect4f UIList::getOptionRect(int curOption) const
{
	if (getNumberOfItems() == 0) {
		return Rect4f();
	} else {
		const auto item = getItem(clamp(curOption, 0, int(getNumberOfItems()) - 1));
		return item->getRawRect() - getPosition();
	}
}

void UIList::onManualControlCycleValue(int delta)
{
	const int start = curOption;
	const int nItems = int(getNumberOfItems());
	for (int i = 1; i < nItems; ++i) {
		int opt = modulo(start + delta * i, nItems);
		if (setSelectedOption(opt)) {
			return;
		}
	}
}

void UIList::onManualControlActivate()
{
	onAccept();
}

void UIList::readFromDataBind()
{
	auto data = getDataBind();
	if (data->getFormat() == UIDataBind::Format::String) {
		setSelectedOptionId(data->getStringData());
	} else {
		setSelectedOption(data->getIntData());
	}
}

bool UIList::isSingleClickAccept() const
{
	return singleClickAccept;
}

void UIList::setSingleClickAccept(bool enabled)
{
	singleClickAccept = enabled;
}
