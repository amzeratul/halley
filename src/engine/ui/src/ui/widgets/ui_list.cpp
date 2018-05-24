#include "widgets/ui_list.h"
#include "ui_style.h"
#include "widgets/ui_label.h"

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

	if (items.empty()) {
		return false;
	}

	auto newSel = clamp(option, 0, int(items.size()) - 1);
	if (newSel != curOption) {
		if (!items.at(newSel)->isEnabled()) {
			return false;
		}

		if (curOption >= 0) {
			items[curOption]->setSelected(false);
		}
		curOption = newSel;
		items[curOption]->setSelected(true);

		playSound(style.getAudioClip("selectionChangedSound"));
		sendEvent(UIEvent(UIEventType::ListSelectionChanged, getId(), items[curOption]->getId(), curOption));
		sendEvent(UIEvent(UIEventType::MakeAreaVisible, getId(), getOptionRect(curOption)));
		return true;
	}
	return false;
}

int UIList::getSelectedOption() const
{
	return curOption;
}

const String& UIList::getSelectedOptionId() const
{
	return items[curOption]->getId();
}

size_t UIList::getCount() const
{
	return items.size();
}

void UIList::addTextItem(const String& id, const LocalisedString& label, float maxWidth)
{
	auto widget = std::make_shared<UILabel>(style.getTextRenderer("label"), label);
	if (maxWidth > 0) {
		widget->setMaxWidth(maxWidth);
	}
	widget->setSelectable(style.getTextRenderer("label").getColour(), style.getTextRenderer("selectedLabel").getColour());

	auto item = std::make_shared<UIListItem>(id, *this, style.getSubStyle("item"), int(items.size()), style.getBorder("extraMouseBorder"));
	item->add(widget, 0);
	addItem(item);
}

void UIList::addItem(const String& id, std::shared_ptr<UIWidget> widget, float proportion, Vector4f border, int fillFlags)
{
	auto item = std::make_shared<UIListItem>(id, *this, style.getSubStyle("item"), int(items.size()), style.getBorder("extraMouseBorder"));
	item->add(widget, proportion, border, fillFlags);
	addItem(item);
}

void UIList::addItem(const String& id, std::shared_ptr<UISizer> sizer, float proportion, Vector4f border, int fillFlags)
{
	auto item = std::make_shared<UIListItem>(id, *this, style.getSubStyle("item"), int(items.size()), style.getBorder("extraMouseBorder"));
	item->add(sizer, proportion, border, fillFlags);
	addItem(item);
}

void UIList::clear()
{
	items.clear();
	curOption = -1;
	curOptionHighlight = -1;
	getSizer().clear();
}

void UIList::setItemEnabled(const String& id, bool enabled)
{
	for (auto& item: items) {
		if (item->getId() == id) {
			item->setEnabled(enabled);
		}
	}
}

void UIList::addItem(std::shared_ptr<UIListItem> item)
{
	add(item);
	bool wasEmpty = items.empty();
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
	sendEvent(UIEvent(UIEventType::ListAccept, getId(), items[curOption]->getId(), curOption));
}

void UIList::onInput(const UIInputResults& input, Time time)
{
	if (items.empty()) {
		curOption = 0;
		return;
	}

	Expects(nColumns >= 1);

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
	option = modulo(option, int(items.size()));

	Vector2i cursorPos;
	if (orientation == UISizerType::Horizontal) {
		nRows = 1;
		nCols = int(items.size());
		cursorPos = Vector2i(option, 0);
	} else if (orientation == UISizerType::Vertical) {
		nRows = int(items.size());
		nCols = 1;
		cursorPos = Vector2i(0, option);
	} else {
		nRows = int(items.size() + nColumns - 1) / nColumns;
		nCols = nColumns;
		cursorPos = Vector2i(option % nCols, option / nCols);
	}

	// Arrows
	cursorPos.x += input.getAxisRepeat(UIInput::Axis::X);
	cursorPos.y += input.getAxisRepeat(UIInput::Axis::Y);
	cursorPos.y = modulo(cursorPos.y, nRows);
	int columnsThisRow = (cursorPos.y == nRows - 1) ? int(items.size()) % nCols : nCols;
	if (columnsThisRow == 0) { // If the last column is full, this will happen
		columnsThisRow = nCols;
	}
	cursorPos.x = modulo(cursorPos.x, columnsThisRow); // The last row has fewer elements

	// Actually update the selection, if it changed
	setSelectedOption(cursorPos.x + cursorPos.y * nCols);

	if (input.isButtonPressed(UIInput::Button::Accept)) {
		onAccept();
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
	parent.onItemClicked(*this);
}

void UIListItem::setSelected(bool s)
{
	if (selected != s) {
		selected = s;
		doSetState(getCurState());

		sendEventDown(UIEvent(UIEventType::SetSelected, getId(), selected));
	}
}

void UIListItem::draw(UIPainter& painter) const
{
	if (sprite.hasMaterial()) {
		painter.draw(sprite);
	}
}

void UIListItem::update(Time t, bool moved)
{
	bool dirty = updateButton() | moved;
	if (dirty) {
		updateSpritePosition();
	}
}

void UIListItem::doSetState(State state)
{
	if (selected) {
		sprite = style.getSprite("selected");
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
		sprite.scaleTo(getSize()).setPos(getPosition());
	}
}

int UIListItem::getIndex() const
{
	return index;
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

bool UIList::setSelectedOptionId(const String& id)
{
	for (auto& i: items) {
		if (i->getId() == id) {
			setSelectedOption(i->getIndex());
			return true;
		}
	}
	return false;
}

Rect4f UIList::getOptionRect(int curOption) const
{
	if (items.empty()) {
		return Rect4f();
	} else {
		auto& item = items[clamp(curOption, 0, int(items.size()) - 1)];
		return item->getRawRect() - getPosition();
	}
}

void UIList::onManualControlCycleValue(int delta)
{
	int start = curOption;
	for (int i = 1; i < int(items.size()); ++i) {
		int opt = modulo(start + delta * i, int(items.size()));
		if (setSelectedOption(opt)) {
			return;
		}
	}
}

void UIList::onManualControlActivate()
{
	onAccept();
}
