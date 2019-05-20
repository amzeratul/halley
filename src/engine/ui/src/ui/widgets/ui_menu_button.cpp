#include "widgets/ui_menu_button.h"
#include "halley/support/logger.h"

using namespace Halley;

UIMenuButton::UIMenuButton(std::shared_ptr<UIMenuButtonGroup> group, String id, Vector2f minSize, Maybe<UISizer> sizer, Vector4f innerBorder)
	: UIClickable(id, minSize, std::move(sizer), innerBorder)
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
	if (isEnabled()) {
		sendEvent(UIEvent(UIEventType::ButtonClicked, getId()));
	}
}

void UIMenuButton::setGroupFocused(bool focused)
{
	groupFocused = focused;
	onGroupState(groupFocused ? State::Hover : State::Up);
}

bool UIMenuButton::isGroupFocused() const
{
	return groupFocused;
}

void UIMenuButton::setOnGroupStateCallback(OnGroupStateCallback callback)
{
	onGroupStateCallback = callback;
}

void UIMenuButton::update(Time t, bool moved)
{
	updateButton();
}

void UIMenuButton::doSetState(State state)
{
	switch (state) {
	case State::Down:
	case State::Hover:
		group->setFocus(*this);
		break;

	default:
		group->setFocusLost(*this);
		break;
	}
}

void UIMenuButton::onGroupState(State state)
{
	if (onGroupStateCallback) {
		onGroupStateCallback(state);
	}
}

void UIMenuButtonGroup::addButton(std::shared_ptr<UIMenuButton> button, const String& up, const String& down, const String& left, const String& right)
{
	buttons.push_back({ button, button->getId(), up, down, left, right });
	if (buttons.size() == 1 && mandatoryFocus) {
		setFocus(*button);
	}
}

void UIMenuButtonGroup::setCancelId(const String& id)
{
	cancelId = id;
}

void UIMenuButtonGroup::clear()
{
	buttons.clear();
}

void UIMenuButtonGroup::onInput(const UIInputResults& input, Time time)
{
	if (size() == 0 || !enabled) {
		return;
	}

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
		auto curFocus = getCurrentFocus();
		if (curFocus) {
			curFocus->onOptionChosen();
		}
	} else if (input.isButtonPressed(UIInput::Button::Cancel)) {
		if (setFocus(cancelId)) {
			auto curFocus = getCurrentFocus();
			if (curFocus) {
				curFocus->onOptionChosen();
			}
		}
	}
}

bool UIMenuButtonGroup::setFocus(UIMenuButton& uiMenuButton)
{
	if (!enabled) {
		return false;
	}

	auto iter = std::find_if(buttons.begin(), buttons.end(), [&] (const ButtonEntry& e) { return e.button.lock().get() == &uiMenuButton; });
	if (iter == buttons.end()) {
		return false;
	}

	if (curFocus != uiMenuButton.getId()) {
		auto cur = getCurrentFocus();
		if (cur) {
			cur->setGroupFocused(false);
		}

		curFocus = uiMenuButton.getId();
		if (!curFocus.isEmpty()) {
			lastFocus = curFocus;
		}

		uiMenuButton.setGroupFocused(true);
	}
	return true;
}

void UIMenuButtonGroup::setFocusLost(UIMenuButton& uiMenuButton)
{
	if (curFocus == uiMenuButton.getId()) {
		setFocusLost();
	}
}

void UIMenuButtonGroup::setFocusLost()
{
	if (!mandatoryFocus) {
		curFocus = "";
	}
}

bool UIMenuButtonGroup::setFocus(const String& id)
{
	if (enabled) {
		for (auto& b: buttons) {
			if (b.id == id) {
				return setFocus(*b.button.lock());
			}
		}

		getCurrentFocus()->sendEvent(UIEvent(UIEventType::GroupFocusChangeRequested, getCurrentFocusId(), id));
	}
	return false;
}

void UIMenuButtonGroup::setFocusOnAnythingValid()
{
	if (curFocus.isEmpty()) {
		if (!lastFocus.isEmpty()) {
			setFocus(lastFocus);
		} else {
			setFocus(buttons.at(0).id);
		}
	}
}

std::shared_ptr<UIMenuButton> UIMenuButtonGroup::getCurrentFocus() const
{
	if (!mandatoryFocus && curFocus.isEmpty()) {
		return {};
	}
	return getCurFocusEntry().button.lock();
}

const String& UIMenuButtonGroup::getCurrentFocusId() const
{
	return curFocus;
}

int UIMenuButtonGroup::getCurrentFocusIndex() const
{
	for (auto i = 0; i < int(buttons.size()); i++)
	{
		if (buttons.at(i).id == curFocus) {
			return i;
		}
	}
	return -1;
}

size_t UIMenuButtonGroup::size() const
{
	return buttons.size();
}

void UIMenuButtonGroup::setEnabled(bool e)
{
	enabled = e;
}

void UIMenuButtonGroup::setMandatoryFocus(bool mandatory)
{
	mandatoryFocus = mandatory;
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

void UIMenuButtonControlWidget::setGroup(std::shared_ptr<UIMenuButtonGroup> group)
{
	this->group = group;
}

void UIMenuButtonControlWidget::onInput(const UIInputResults& input, Time time)
{
	group->onInput(input, time);
}

UIMenuButtonGroupHighlight::UIMenuButtonGroupHighlight(std::shared_ptr<UIMenuButtonGroup> group, Time transitionAnimLen)
	: group(group)
	, transitionAnimLen(transitionAnimLen)
{
}

void UIMenuButtonGroupHighlight::setFocusChangedCallback(FocusChangedCallback callback)
{
	focusChangedCallback = std::move(callback);
}

void UIMenuButtonGroupHighlight::update(Time time)
{
	elapsedTime += time;

	const String curFocus = group->getCurrentFocusId();

	if (lastFocus != curFocus) {
		transitionTime = lastFocus.isEmpty() ? transitionAnimLen : 0;
		auto prevFocus = lastFocus;
		lastFocus = curFocus;
		prevRect = targetRect;
		onFocusChanged(curFocus, prevFocus);
	}

	transitionTime += time;
	const auto focused = group->getCurrentFocus();
	if (focused) {
		targetRect = focused->getMouseRect();
	}

	const float t = smoothCos(clamp(float(transitionTime / transitionAnimLen), 0.0f, 1.0f));
	curRect = lerp(prevRect, targetRect, t);
}

Rect4f UIMenuButtonGroupHighlight::getCurRect() const
{
	return curRect;
}

Time UIMenuButtonGroupHighlight::getElapsedTime() const
{
	return elapsedTime;
}

UIMenuButtonGroup& UIMenuButtonGroupHighlight::getGroup()
{
	return *group;
}

void UIMenuButtonGroupHighlight::onFocusChanged(const String& id, const String& previous)
{
	if (focusChangedCallback) {
		focusChangedCallback(id, previous.isEmpty());
	}
}
