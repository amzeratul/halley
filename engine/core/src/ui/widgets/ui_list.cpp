#include "ui/widgets/ui_list.h"
#include "ui/ui_style.h"
#include "ui/widgets/ui_label.h"

using namespace Halley;

UIList::UIList(const String& id, std::shared_ptr<UIStyle> style, UISizerType orientation)
	: UIWidget(id, {}, UISizer(orientation))
	, style(style)
	, orientation(orientation)
{
	sprite = style->listBackground;
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

		sendEvent(UIEvent(UIEventType::ListSelectionChanged, getId(), items[curOption]->getId()));
	}
}

int UIList::getSelectedOption() const
{
	return curOption;
}

void UIList::addTextItem(const String& id, const String& label)
{
	auto widget = std::make_shared<UILabel>(style->label.clone().setText(label));
	auto item = std::make_shared<UIListItem>(id, *this, style, int(items.size()));
	item->add(widget, 0, orientation == UISizerType::Vertical ? Vector4f(3, 0, 3, 0) : Vector4f(0, 3, 0, 3));
	addItem(item);
}

void UIList::addItem(const String& id, std::shared_ptr<UIWidget> widget, float proportion, Vector4f border, int fillFlags)
{
	auto item = std::make_shared<UIListItem>(id, *this, style, int(items.size()));
	item->add(widget, proportion, border, fillFlags);
	addItem(item);
}

void UIList::addItem(const String& id, std::shared_ptr<UISizer> sizer, float proportion, Vector4f border, int fillFlags)
{
	auto item = std::make_shared<UIListItem>(id, *this, style, int(items.size()));
	item->add(sizer, proportion, border, fillFlags);
	addItem(item);
}

void UIList::setInputButtons(const Buttons& button)
{
	inputButtons = button;
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

void UIList::updateInputDevice(InputDevice& device)
{
	auto checkButton = [&] (int button) { return button >= 0 && device.isButtonPressed(button); };

	int x = device.getAxisRepeat(0);
	int y = device.getAxisRepeat(1);
	
	int newSel = curOption;

	if (inputButtons.xAxis && x) {
		newSel += x;
	}
	if (inputButtons.yAxis && y) {
		newSel += y;
	}
	if (checkButton(inputButtons.next)) {
		newSel++;
	}
	if (checkButton(inputButtons.prev)) {
		newSel--;
	}

	setSelectedOption(newSel);

	if (checkButton(inputButtons.accept)) {
		sendEvent(UIEvent(UIEventType::ListAccept, getId(), items[curOption]->getId()));
	}
}

void UIList::update(Time t, bool moved)
{
	if (moved) {
		if (sprite.hasMaterial()) {
			sprite.scaleTo(getSize()).setPos(getPosition());
		}
	}
}

void UIList::onItemClicked(UIListItem& item)
{
	setSelectedOption(item.getIndex());
}

UIListItem::UIListItem(const String& id, UIList& parent, std::shared_ptr<UIStyle> style, int index)
	: UIClickable(id, {}, UISizer(UISizerType::Vertical))
	, parent(parent)
	, style(style)
	, index(index)
{
	sprite = style->listItemNormal;
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
		sprite = style->listItemSelected;
	} else {
		switch (state) {
		case State::Up:
			sprite = style->listItemNormal;
			break;
		case State::Hover:
			sprite = style->listItemHover;
			break;
		case State::Down:
			sprite = style->listItemSelected;
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
