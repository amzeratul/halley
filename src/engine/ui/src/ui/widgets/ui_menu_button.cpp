#include "widgets/ui_menu_button.h"

using namespace Halley;

UIMenuButton::UIMenuButton(std::shared_ptr<UIMenuButtonGroup> group, String id, Vector2f minSize, Maybe<UISizer> sizer, Vector4f innerBorder)
	: UIClickable(id, minSize, sizer, innerBorder)
	, group(group)
{
	Expects(group);
}

void UIMenuButton::onClicked(Vector2f mousePos)
{
	onOptionChosen();
}

void UIMenuButton::onOptionChosen()
{
	sendEvent(UIEvent(UIEventType::ButtonClicked, getId()));
}

void UIMenuButton::setGroupFocused(bool focused)
{
	groupFocused = focused;
	onGroupState(groupFocused ? State::Hover : State::Up);
}

void UIMenuButton::doSetState(State state)
{
	switch (state) {
	case State::Down:
	case State::Hover:
		group->setFocus(*this);
		break;

	default:
		break;
	}
}

void UIMenuButton::onGroupState(State state)
{
}

void UIMenuButtonGroup::addButton(std::shared_ptr<UIMenuButton> button, const String& up, const String& down, const String& left, const String& right)
{
	buttons.push_back({ button, button->getId(), up, down, left, right });
}

void UIMenuButtonGroup::setCancelId(const String& id)
{
	cancelId = id;
}

void UIMenuButtonGroup::onInput(const UIInputResults& input)
{
	const int x = input.getAxisRepeat(UIInput::Axis::X);
	const int y = input.getAxisRepeat(UIInput::Axis::Y);

	auto& cur = getCurFocusEntry();

	if (y != 0) {
		if (y == -1 && !cur.up.isEmpty()) {
			setFocus(cur.up);
		} else if (y == 1 && !cur.down.isEmpty()) {
			setFocus(cur.down);
		}
	}
	
	if (cur.id == curFocus) { // Only check X axis if Y didn't result in a focus change
		if (x != 0) {
			if (x == -1 && !cur.left.isEmpty()) {
				setFocus(cur.left);
			} else if (x == 1 && !cur.right.isEmpty()) {
				setFocus(cur.right);
			}
		}
	}

	if (input.isButtonPressed(UIInput::Button::Accept)) {
		getCurrentFocus()->onOptionChosen();
	} else if (input.isButtonPressed(UIInput::Button::Cancel)) {
		if (setFocus(cancelId)) {
			getCurrentFocus()->onOptionChosen();
		}
	}
}

bool UIMenuButtonGroup::setFocus(UIMenuButton& uiMenuButton)
{
	auto iter = std::find_if(buttons.begin(), buttons.end(), [&] (const ButtonEntry& e) { return e.button.lock().get() == &uiMenuButton; });
	if (iter == buttons.end()) {
		return false;
	}

	if (curFocus != uiMenuButton.getId()) {
		getCurrentFocus()->setGroupFocused(false);

		curFocus = uiMenuButton.getId();

		uiMenuButton.setGroupFocused(true);
	}
	return true;
}

bool UIMenuButtonGroup::setFocus(const String& id)
{
	for (auto& b: buttons) {
		if (b.id == id) {
			return setFocus(*b.button.lock());
		}
	}
	return false;
}

std::shared_ptr<UIMenuButton> UIMenuButtonGroup::getCurrentFocus() const
{
	return getCurFocusEntry().button.lock();
}

const UIMenuButtonGroup::ButtonEntry& UIMenuButtonGroup::getCurFocusEntry() const
{
	for (auto& b: buttons) {
		if (b.id == curFocus) {
			return b;
		}
	}
	return buttons.at(0);
}

UIMenuButtonGroup::ButtonEntry& UIMenuButtonGroup::getCurFocusEntry()
{
	for (auto& b: buttons) {
		if (b.id == curFocus) {
			return b;
		}
	}
	return buttons.at(0);
}

UIMenuButtonControlWidget::UIMenuButtonControlWidget(std::shared_ptr<UIMenuButtonGroup> group)
	: UIWidget("groupControl", Vector2f())
	, group(std::move(group))
{
}

void UIMenuButtonControlWidget::onInput(const UIInputResults& input)
{
	group->onInput(input);
}
