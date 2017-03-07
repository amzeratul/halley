#include "ui/widgets/ui_list.h"
#include "ui/ui_style.h"
#include "ui/widgets/ui_label.h"

using namespace Halley;

UIList::UIList(const String& id, std::shared_ptr<UIStyle> style)
	: UIWidget(id, {}, UISizer(UISizerType::Vertical))
	, style(style)
{
	sprite = style->listBackground;
}

void UIList::setSelectedOption(int option)
{
	curOption = option;
}

int UIList::getSelectedOption() const
{
	return curOption;
}

void UIList::addTextItem(const String& id, const String& label)
{
	addItem(id, std::make_unique<UILabel>(style->label.clone().setText(label)));
}

void UIList::addItem(const String& id, std::shared_ptr<UIWidget> widget)
{
}

void UIList::draw(UIPainter& painter) const
{
	if (sprite.hasMaterial()) {
		painter.draw(sprite);
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

UIListItem::UIListItem(const String& id, UIList& parent, std::shared_ptr<UIStyle> style)
	: UIClickable(id, {})
	, parent(parent)
{
}

void UIListItem::onClicked(Vector2f mousePos)
{
}

void UIListItem::doSetState(State state)
{
}
