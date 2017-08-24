#include "ui/widgets/ui_list.h"
#include "ui/ui_style.h"
#include "ui/widgets/ui_label.h"

using namespace Halley;

UIList::UIList(const String& id, std::shared_ptr<UIStyle> style, UISizerType orientation, int nColumns)
	: UIWidget(id, {}, UISizer(orientation, style->getFloat("list.gap"), nColumns, true))
	, style(style)
	, orientation(orientation)
	, nColumns(nColumns)
{
	sprite = style->getSprite("list.background");
}

void UIList::setSelectedOption(int option)
{
	auto newSel = modulo(option, int(items.size()));
	if (newSel != curOption) {
		if (curOption >= 0) {
			items[curOption]->setSelected(false);
		}
		curOption = newSel;
		items[curOption]->setSelected(true);

		playSound(style->getAudioClip("list.selectionChangedSound"));
		sendEvent(UIEvent(UIEventType::ListSelectionChanged, getId(), items[curOption]->getId(), curOption));
		sendEvent(UIEvent(UIEventType::MakeAreaVisible, getId(), getOptionRect(curOption)));
	}
}

int UIList::getSelectedOption() const
{
	return curOption;
}

const String& UIList::getSelectedOptionId() const
{
	return items[curOption]->getId();
}

void UIList::addTextItem(const String& id, const String& label)
{
	auto widget = std::make_shared<UILabel>(style->getTextRenderer("label").clone().setText(label));
	auto item = std::make_shared<UIListItem>(id, *this, style, int(items.size()), style->getBorder("list.extraMouseBorder"));
	item->add(widget, 0, orientation == UISizerType::Vertical ? Vector4f(3, 0, 3, 0) : Vector4f(0, 3, 0, 3));
	addItem(item);
}

void UIList::addItem(const String& id, std::shared_ptr<UIWidget> widget, float proportion, Vector4f border, int fillFlags)
{
	auto item = std::make_shared<UIListItem>(id, *this, style, int(items.size()), style->getBorder("list.extraMouseBorder"));
	item->add(widget, proportion, border, fillFlags);
	addItem(item);
}

void UIList::addItem(const String& id, std::shared_ptr<UISizer> sizer, float proportion, Vector4f border, int fillFlags)
{
	auto item = std::make_shared<UIListItem>(id, *this, style, int(items.size()), style->getBorder("list.extraMouseBorder"));
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

void UIList::addItem(std::shared_ptr<UIListItem> item)
{
	add(item);
	items.push_back(item);
	if (curOption == -1) {
		setSelectedOption(0);
	}
}

void UIList::draw(UIPainter& painter) const
{
	if (sprite.hasMaterial()) {
		painter.draw(sprite);
	}
}


void UIList::onInput(const UIInputResults& input)
{
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
		sendEvent(UIEvent(UIEventType::ListAccept, getId(), items[curOption]->getId(), curOption));
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

UIListItem::UIListItem(const String& id, UIList& parent, std::shared_ptr<UIStyle> style, int index, Vector4f extraMouseArea)
	: UIClickable(id, {}, UISizer(UISizerType::Vertical))
	, parent(parent)
	, style(style)
	, index(index)
	, extraMouseArea(extraMouseArea)
{
	sprite = style->getSprite("list.itemNormal");
}

void UIListItem::onClicked(Vector2f mousePos)
{
	parent.onItemClicked(*this);
}

void UIListItem::setSelected(bool s)
{
	if (selected != s) {
		selected = s;
		parent.curOption = selected ? index : -1;
		doSetState(getCurState());
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
		sprite = style->getSprite("list.itemSelected");
	} else {
		switch (state) {
		case State::Up:
			sprite = style->getSprite("list.itemNormal");
			break;
		case State::Hover:
			sprite = style->getSprite("list.itemHover");
			break;
		case State::Down:
			sprite = style->getSprite("list.itemSelected");
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

void UIList::setSelectedOptionId(const String& id)
{
	for (auto& i: items) {
		if (i->getId() == id) {
			setSelectedOption(i->getIndex());
			return;
		}
	}
}

Rect4f UIList::getOptionRect(int curOption) const
{
	Expects(!items.empty());
	auto& item = items[clamp(curOption, 0, int(items.size()) - 1)];
	return item->getRawRect() - getPosition();
}
