#include "ui/ui_root.h"
#include "ui/ui_widget.h"
#include "api/audio_api.h"
#include "ui/ui_painter.h"

using namespace Halley;

UIRoot* UIRoot::getRoot()
{
	return this;
}

UIRoot::UIRoot(AudioAPI* audio)
	: dummyInput(std::make_shared<InputButtonBase>(4))
	, audio(audio)
{
}

void UIRoot::update(Time t, UIInputType activeInputType, spInputDevice mouse, spInputDevice manual, Vector2f uiOffset)
{
	// Update input
	updateTabbing(manual);
	updateMouse(mouse, uiOffset);

	// Update children
	{
		bool allowInput = true;
		auto& cs = getChildren();
		for (int i = int(cs.size()); --i >= 0; ) {
			cs[i]->doUpdate(t, activeInputType, allowInput ? *manual : *dummyInput);
			if (cs[i]->isMouseBlocker()) {
				allowInput = false;
			}
		}
	}

	// Layout all widgets
	runLayout();

	// Remove dead
	removeDeadChildren();
	bool added = addNewChildren(activeInputType);

	// Update new windows
	if (topChildChanged) {
		topChildChanged = false;
		if (activeInputType == UIInputType::Mouse) {
			lastMousePos = Vector2f(-100, -100);
		} else {
			//mouseOverNext();
		}
	}

	// Update again, to reflect what happened >_>
	runLayout();
	for (auto& c: getChildren()) {
		c->doUpdate(0, activeInputType, *dummyInput);
	}
}

void UIRoot::updateMouse(spInputDevice mouse, Vector2f uiOffset)
{
	// Check where we should be mouse overing.
	// If the mouse hasn't moved, keep the last one.
	std::shared_ptr<UIWidget> underMouse;
	Vector2f mousePos = mouse->getPosition() + uiOffset;
	if ((mousePos - lastMousePos).squaredLength() > 0.01f) {
		// Go through all root-level widgets and find the actual widget under the mouse
		underMouse = getWidgetUnderMouse(mousePos);
		lastMousePos = mousePos;
	} else {
		underMouse = currentMouseOver.lock();
	}

	// Click
	if (mouse->isButtonPressed(0)) {
		mouseHeld = true;
		setFocus(underMouse);
		if (underMouse) {
			underMouse->pressMouse(mousePos, 0);
		}
	}

	// Release click
	auto focus = currentFocus.lock();
	if (mouse->isButtonReleased(0)) {
		mouseHeld = false;
		if (focus) {
			focus->releaseMouse(mousePos, 0);
		}
	}

	// If the mouse is held, but it's over a different component from the focused one, don't mouse over anything
	auto activeMouseOver = underMouse;
	if (mouseHeld && focus && focus != underMouse) {
		activeMouseOver.reset();
	}
	updateMouseOver(activeMouseOver);
}

void UIRoot::updateTabbing(spInputDevice manual)
{
	int x = manual->getAxisRepeat(0);
	int y = manual->getAxisRepeat(1);

	/*	
	if (x > 0 || y > 0) {
		mouseOverNext(true);
	} else if (x < 0 || y < 0) {
		mouseOverNext(false);
	}
	*/
}

void UIRoot::mouseOverNext(bool forward)
{
	auto widgets = collectWidgets();

	if (widgets.empty()) {
		return;
	}

	size_t nextIdx = 0;
	auto current = currentMouseOver.lock();
	if (current) {
		auto i = std::find(widgets.begin(), widgets.end(), current);
		if (i != widgets.end()) {
			nextIdx = ((i - widgets.begin()) + widgets.size() + (forward ? 1 : -1)) % widgets.size();
		}
	}

	updateMouseOver(widgets[nextIdx]);
}

void UIRoot::runLayout()
{
	for (auto& c: getChildren()) {
		c->layout();
	}
}

void UIRoot::setFocus(std::shared_ptr<UIWidget> focus)
{
	auto curFocus = currentFocus.lock();
	if (curFocus != focus) {
		if (curFocus) {
			curFocus->setFocused(false, focus.get());
		}

		currentFocus = focus;
		if (focus) {
			focus->setFocused(true, focus.get());
		}
	}
}

void UIRoot::updateMouseOver(const std::shared_ptr<UIWidget>& underMouse)
{
	auto curMouseOver = currentMouseOver.lock();
	if (curMouseOver != underMouse) {
		if (curMouseOver) {
			curMouseOver->setMouseOver(false);
		}
		if (underMouse) {
			underMouse->setMouseOver(true);
		}
		currentMouseOver = underMouse;
	}
}


std::shared_ptr<UIWidget> UIRoot::getWidgetUnderMouse(Vector2f mousePos)
{
	auto& cs = getChildren();
	for (int i = int(cs.size()); --i >= 0; ) {
		auto widget = getWidgetUnderMouse(cs[i], mousePos);
		if (widget) {
			return widget;
		} else {
			if (cs[i]->isMouseBlocker()) {
				return {};
			}
		}
	}
	return {};
}

std::shared_ptr<UIWidget> UIRoot::getWidgetUnderMouse(const std::shared_ptr<UIWidget>& start, Vector2f mousePos)
{
	// Depth first
	for (auto& c: start->getChildren()) {
		auto result = getWidgetUnderMouse(c, mousePos);
		if (result) {
			return result;
		}
	}

	auto rect = start->getMouseRect();
	if (start->isFocusable() && rect.isInside(mousePos)) {
		return start;
	} else {
		return {};
	}
}

std::vector<std::shared_ptr<UIWidget>> UIRoot::collectWidgets()
{
	std::vector<std::shared_ptr<UIWidget>> output;
	if (getChildren().empty()) {
		return {};
	}
	collectWidgets(getChildren().back(), output);
	return output;
}

void UIRoot::collectWidgets(const std::shared_ptr<UIWidget>& start, std::vector<std::shared_ptr<UIWidget>>& output)
{
	for (auto& c: start->getChildren()) {
		collectWidgets(c, output);
	}

	if (start->isFocusable()) {
		output.push_back(start);
	}
}

void UIRoot::draw(SpritePainter& painter, int mask, int layer)
{
	UIPainter p(painter, mask, layer);

	for (auto& c: getChildren()) {
		c->doDraw(p);
	}
}

void UIRoot::playSound(const std::shared_ptr<const AudioClip>& clip)
{
	if (audio && clip) {
		audio->playUI(clip);
	}
}

void UIRoot::sendEvent(UIEvent&&) const
{
	// Unhandled event
}

bool UIRoot::hasModalUI() const
{
	for (auto& c: getChildren()) {
		if (c->isModal()) {
			return true;
		}
	}
	return false;
}

bool UIRoot::isMouseOverUI() const
{
	return static_cast<bool>(currentMouseOver.lock());
}

UIWidget* UIRoot::getCurrentFocus() const
{
	return currentFocus.lock().get();
}
