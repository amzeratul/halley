#include "widgets/ui_list.h"
#include "ui_style.h"
#include "widgets/ui_label.h"
#include "halley/support/logger.h"

using namespace Halley;

UIList::UIList(const String& id, UIStyle style, UISizerType orientation, int nColumns)
	: UIWidget(id, {}, UISizer(orientation, style.getFloat("gap"), nColumns), style.getBorder("innerBorder"))
	, style(style)
	, orientation(orientation)
	, nColumns(nColumns)
{
	getSizer().setEvenColumns();
	sprite = style.getSprite("background");

	setHandle(UIEventType::SetSelected, [=] (const UIEvent& event) {});
	setHandle(UIEventType::SetHovered, [=] (const UIEvent& event) {});
}

bool UIList::setSelectedOption(int option)
{
	forceAddChildren(UIInputType::Undefined);

	const auto numberOfItems = int(getNumberOfItems());
	if (numberOfItems == 0) {
		return false;
	}

	auto newSel = clamp(option, 0, numberOfItems - 1);
	if (newSel != curOption) {
		if (!getItem(newSel)->isEnabled()) {
			return false;
		}

		if (curOption >= 0 && curOption < numberOfItems) {
			getItem(curOption)->setSelected(false);
		}
		curOption = newSel;
		auto curItem = getItem(curOption);
		curItem->setSelected(true);

		playSound(style.getString("selectionChangedSound"));

		sendEvent(UIEvent(UIEventType::ListSelectionChanged, getId(), curItem->getId(), curOption));
		sendEvent(UIEvent(UIEventType::MakeAreaVisible, getId(), getOptionRect(curOption)));
		
		if (getDataBindFormat() == UIDataBind::Format::String) {
			notifyDataBind(curItem->getId());
		} else {
			notifyDataBind(curOption);
		}

		return true;
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
	return style;
}

void UIList::addTextItem(const String& id, const LocalisedString& label, float maxWidth, bool centre)
{
	auto widget = std::make_shared<UILabel>(id + "_label", style.getTextRenderer("label"), label);
	if (maxWidth > 0) {
		widget->setMaxWidth(maxWidth);
	}

	if (style.hasTextRenderer("selectedLabel")) {
		widget->setSelectable(style.getTextRenderer("label"), style.getTextRenderer("selectedLabel"));
	}

	if (style.hasTextRenderer("disabledStyle")) {
		widget->setDisablable(style.getTextRenderer("label"), style.getTextRenderer("disabledStyle"));
	}

	auto item = std::make_shared<UIListItem>(id, *this, style.getSubStyle("item"), int(getNumberOfItems()), style.getBorder("extraMouseBorder"));
	item->add(widget, 0, Vector4f(), centre ? UISizerAlignFlags::CentreHorizontal : UISizerFillFlags::Fill);
	addItem(item);
}

void UIList::addItem(const String& id, std::shared_ptr<IUIElement> element, float proportion, Vector4f border, int fillFlags, Maybe<UIStyle> styleOverride)
{
	const auto& itemStyle = styleOverride ? *styleOverride : style;
	auto item = std::make_shared<UIListItem>(id, *this, itemStyle.getSubStyle("item"), int(getNumberOfItems()), itemStyle.getBorder("extraMouseBorder"));
	item->add(element, proportion, border, fillFlags);
	addItem(item);
}

void UIList::clear()
{
	items.clear();
	curOption = -1;
	curOptionHighlight = -1;
	UIWidget::clear();
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
	if (!setSelectedOptionId(curId)) {
		curOption = -1;
		setSelectedOption(0);
	}
}

void UIList::setItemActive(const String& id, bool active)
{
	auto curId = getSelectedOptionId();
	for (auto& item: items) {
		if (item->getId() == id) {
			if (!active) {
				item->setSelected(false);
			}
			item->setActive(active);
		}
	}
	reassignIds();
	if (!setSelectedOptionId(curId)) {
		curOption = -1;
		setSelectedOption(0);
	}
}

void UIList::addItem(std::shared_ptr<UIListItem> item)
{
	add(item, uniformSizedItems ? 1.0f : 0.0f);
	bool wasEmpty = getNumberOfItems() == 0;
	items.push_back(item);
	if (wasEmpty) {
		curOption = -1;
		setSelectedOption(0);
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
	sendEvent(UIEvent(UIEventType::ListAccept, getId(), getItem(curOption)->getId(), curOption));
}

void UIList::onCancel()
{
	sendEvent(UIEvent(UIEventType::ListCancel, getId(), getItem(curOption)->getId(), curOption));
}

void UIList::reassignIds()
{
	int i = 0;
	for (auto& item: items) {
		if (item->isActive() && item->isEnabled()) {
			item->setIndex(i++);
		} else {
			item->setIndex(-1);
		}
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

bool UIList::canDrag() const
{
	return dragEnabled;
}

void UIList::setDrag(bool drag)
{
	dragEnabled = drag;
}

void UIList::setUniformSizedItems(bool enabled)
{
	uniformSizedItems = enabled;
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
	if (curOptionHighlight == idxA) {
		curOptionHighlight = idxB;
	} else if (curOptionHighlight == idxB) {
		curOptionHighlight = idxA;
	}

	std::swap(items[idxA], items[idxB]);
	reassignIds();
	getSizer().swapItems(idxA, idxB);
	items[idxA]->notifySwap(items[idxB]->getOrigPosition());
	items[idxB]->notifySwap(items[idxA]->getOrigPosition());
	sendEvent(UIEvent(UIEventType::ListItemsSwapped, getId(), idxA, idxB));
}

bool UIList::isManualDragging() const
{
	return manualDragging;
}

void UIList::onInput(const UIInputResults& input, Time time)
{
	if (getNumberOfItems() == 0) {
		curOption = 0;
		return;
	}

	Expects(nColumns >= 1);

	// Drag
	if (dragEnabled && input.isButtonHeld(UIInput::Button::Hold)) {
		// Manual dragging
		manualDragging = true;

		int dir = 0;
		if (input.isButtonPressed(UIInput::Button::Next)) {
			dir++;
		} else if (input.isButtonPressed(UIInput::Button::Prev)) {
			dir--;
		} else if (orientation == UISizerType::Horizontal) {
			dir += input.getAxisRepeat(UIInput::Axis::X);
		} else if (orientation == UISizerType::Vertical) {
			dir += input.getAxisRepeat(UIInput::Axis::Y);
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
		int nCols;
		int nRows;
		int option = curOption;

		// Next/prev first
		if (input.isButtonPressed(UIInput::Button::Next)) {
			option++;
		}
		if (input.isButtonPressed(UIInput::Button::Prev)) {
			option--;
		}
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
		cursorPos.x += input.getAxisRepeat(UIInput::Axis::X);
		cursorPos.y += input.getAxisRepeat(UIInput::Axis::Y);
		cursorPos.y = modulo(cursorPos.y, nRows);
		int columnsThisRow = (cursorPos.y == nRows - 1) ? nItems % nCols : nCols;
		if (columnsThisRow == 0) { // If the last column is full, this will happen
			columnsThisRow = nCols;
		}
		cursorPos.x = modulo(cursorPos.x, columnsThisRow); // The last row has fewer elements

		// Actually update the selection, if it changed
		setSelectedOption(cursorPos.x + cursorPos.y * nCols);
	}

	if (input.isButtonPressed(UIInput::Button::Accept)) {
		onAccept();
	}

	if (input.isButtonPressed(UIInput::Button::Cancel)) {
		onCancel();
	}
}

void UIList::update(Time t, bool moved)
{
	if (moved) {
		if (sprite.hasMaterial()) {
			sprite.scaleTo(getSize()).setPos(getPosition());
		}
	}

	if (firstUpdate) {
		sendEvent(UIEvent(UIEventType::MakeAreaVisibleCentered, getId(), getOptionRect(curOption)));
		firstUpdate = false;
	}
}

void UIList::onItemClicked(UIListItem& item)
{
	setSelectedOption(item.getIndex());
	sendEvent(UIEvent(UIEventType::ListAccept, getId(), curOption));
}

void UIList::onItemDragged(UIListItem& item, int index, Vector2f pos)
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
		if (next->canSwap() && pos[axis] > next->getPosition()[axis] - 2.0f) {
			swapItems(index, index + 1);
		}
	}
}

UIListItem::UIListItem(const String& id, UIList& parent, UIStyle style, int index, Vector4f extraMouseArea)
	: UIClickable(id, {}, UISizer(UISizerType::Vertical), style.getBorder("innerBorder"))
	, parent(parent)
	, style(style)
	, index(index)
	, extraMouseArea(extraMouseArea)
{
	sprite = style.getSprite("normal");
}

void UIListItem::onClicked(Vector2f mousePos)
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

void UIListItem::setStyle(UIStyle style)
{
	this->style = style;
	doSetState(getCurState());
}

void UIListItem::onEnabledChanged()
{
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
		
		const auto parentRect = parent.getRect();
		const auto myTargetRect = Rect4f(curDragPos, curDragPos + getSize());
		setPosition(myTargetRect.fitWithin(parentRect).getTopLeft());
		layout();
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

	bool manualDragging = isManualDragging();
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
	if (parent.canDrag() && held /* && (mousePos - mouseStartPos).length() > 2.0f */) {
		dragged = true;
	}
	if (dragged) {
		setDragPos(mousePos - mouseStartPos + myStartPos);
	}
}

void UIListItem::pressMouse(Vector2f mousePos, int button)
{
	UIClickable::pressMouse(mousePos, button);
	if (button == 0) {
		held = true;
		dragged = false;
		mouseStartPos = mousePos;
		myStartPos = getPosition();
		parent.onItemClicked(*this);
	}
}

void UIListItem::releaseMouse(Vector2f mousePos, int button)
{
	UIClickable::releaseMouse(mousePos, button);
	if (button == 0) {
		if (held) {
			onMouseOver(mousePos);
			held = false;
			dragged = false;
		}
	}
}

void UIListItem::setDragPos(Vector2f pos)
{
	curDragPos = pos.round();
	parent.onItemDragged(*this, index, curDragPos);
}

void UIListItem::doSetState(State state)
{
	if (dragged || isManualDragging()) {
		sprite = style.getSprite("drag");
	} else if (selected) {
		sprite = style.getSprite("selected");
	} else if (!isEnabled()) {
		sprite = style.getSprite("disabled");
	} else {
		switch (state) {
		case State::Up:
			sprite = style.getSprite("normal");
			sendEventDown(UIEvent(UIEventType::SetHovered, getId(), false));
			break;
		case State::Hover:
			sprite = style.getSprite("hover");
			sendEventDown(UIEvent(UIEventType::SetHovered, getId(), true));
			break;
		case State::Down:
			sprite = style.getSprite("selected");
			break;
		}
	}
	updateSpritePosition();
}

void UIListItem::updateSpritePosition()
{
	if (sprite.hasMaterial()) {
		Vector2f pos = getPosition();
		sprite.scaleTo(getSize()).setPos(pos);
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

Rect4f UIListItem::getMouseRect() const
{
	auto rect = UIWidget::getMouseRect();
	if (rect.getWidth() <= 0.01f || rect.getHeight() <= 0.01f) {
		return rect;
	}
	return Rect4f(rect.getTopLeft() - Vector2f(extraMouseArea.x, extraMouseArea.y), rect.getBottomRight() + Vector2f(extraMouseArea.z, extraMouseArea.w));
}

Rect4f UIListItem::getRawRect() const
{
	return Rect4f(getPosition(), getPosition() + getSize());
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

bool UIList::setSelectedOptionId(const String& id)
{
	for (auto& i: items) {
		if (i->getId() == id) {
			if (i->isActive()) {
				setSelectedOption(i->getIndex());
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
