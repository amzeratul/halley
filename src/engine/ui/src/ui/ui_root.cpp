#include "ui_root.h"
#include "ui_widget.h"
#include "halley/core/api/audio_api.h"
#include "ui_painter.h"
#include "halley/audio/audio_position.h"
#include "halley/audio/audio_clip.h"
#include "halley/maths/random.h"

using namespace Halley;

UIRoot* UIRoot::getRoot()
{
	return this;
}

UIRoot::UIRoot(AudioAPI* audio, Rect4f rect)
	: dummyInput(std::make_shared<InputButtonBase>(4))
	, uiRect(rect)
	, audio(audio)
{
}

void UIRoot::setRect(Rect4f rect)
{
	uiRect = rect;
}

Rect4f UIRoot::getRect() const
{
	return uiRect;
}

void UIRoot::update(Time t, UIInputType activeInputType, spInputDevice mouse, spInputDevice manual)
{
	auto joystickType = manual->getJoystickType();

	// Spawn & Update input
	addNewChildren(activeInputType);
	if (activeInputType == UIInputType::Mouse) {
		updateMouse(mouse);
	}
	updateInput(manual);

	// Update children
	for (auto& c: getChildren()) {
		c->doUpdate(true, t, activeInputType, joystickType);
	}
	removeDeadChildren();

	// Layout all widgets
	runLayout();

	// Update again, to reflect what happened >_>
	for (auto& c: getChildren()) {
		c->doUpdate(false, 0, activeInputType, joystickType);
	}
	//addNewChildren(activeInputType);
	runLayout();
}

void UIRoot::updateMouse(spInputDevice mouse)
{
	// Check where we should be mouse overing.
	// If the mouse hasn't moved, keep the last one.
	std::shared_ptr<UIWidget> underMouse;
	Vector2f mousePos = mouse->getPosition() + uiRect.getTopLeft();
	if (true || (mousePos - lastMousePos).squaredLength() > 0.01f) {
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

	// Mouse wheel
	int wheelDelta = mouse->getWheelMove();
	if (wheelDelta != 0 && underMouse) {
		underMouse->sendEvent(UIEvent(UIEventType::MouseWheel, "mouse", wheelDelta));
	}

	// If the mouse is held, but it's over a different component from the focused one, don't mouse over anything
	auto activeMouseOver = underMouse;
	if (mouseHeld && focus) {
		if (focus != underMouse) {
			activeMouseOver.reset();
		}
		focus->onMouseOver(mousePos);
	} else if (activeMouseOver) {
		activeMouseOver->onMouseOver(mousePos);
	}
	updateMouseOver(activeMouseOver);
}

void UIRoot::updateInputTree(const spInputDevice& input, UIWidget& widget, std::vector<UIWidget*>& inputTargets, UIInput::Priority& bestPriority, bool accepting)
{
	if (!widget.isActive()) {
		return;
	}

	for (auto& c: widget.getChildren()) {
		// Depth-first
		updateInputTree(input, *c, inputTargets, bestPriority, accepting);
	}

	if (widget.inputButtons) {
		widget.inputResults.reset();
		if (accepting) {
			auto priority = widget.getInputPriority();

			if (int(priority) > int(bestPriority)) {
				bestPriority = priority;
				inputTargets.clear();
			}
			if (priority == bestPriority) {
				inputTargets.push_back(&widget);
			}
		}
	}
}

void UIRoot::updateInput(spInputDevice input)
{
	auto& cs = getChildren();
	std::vector<UIWidget*> inputTargets;
	UIInput::Priority bestPriority = UIInput::Priority::Lowest;

	bool accepting = true;
	for (int i = int(cs.size()); --i >= 0; ) {
		auto& c = *cs[i];
		updateInputTree(input, c, inputTargets, bestPriority, accepting);
				
		if (c.isMouseBlocker()) {
			accepting = false;
		}
	}

	for (auto& target: inputTargets) {
		auto& b = *target->inputButtons;
		auto& results = target->inputResults;
		results.setButtonPressed(UIInput::Button::Accept, b.accept != -1 ? input->isButtonPressed(b.accept) : false);
		results.setButtonPressed(UIInput::Button::Cancel, b.cancel != -1 ? input->isButtonPressed(b.cancel) : false);
		results.setButtonPressed(UIInput::Button::Prev, b.prev != -1 ? input->isButtonPressed(b.prev) : false);
		results.setButtonPressed(UIInput::Button::Next, b.next != -1 ? input->isButtonPressed(b.next) : false);
		results.setAxis(UIInput::Axis::X, (b.xAxis != -1 ? input->getAxis(b.xAxis) : 0) + (b.xAxisAlt != -1 ? input->getAxis(b.xAxisAlt) : 0));
		results.setAxis(UIInput::Axis::Y, (b.yAxis != -1 ? input->getAxis(b.yAxis) : 0) + (b.yAxisAlt != -1 ? input->getAxis(b.yAxisAlt) : 0));
		results.setAxisRepeat(UIInput::Axis::X, (b.xAxis != -1 ? input->getAxisRepeat(b.xAxis) : 0) + (b.xAxisAlt != -1 ? input->getAxisRepeat(b.xAxisAlt) : 0));
		results.setAxisRepeat(UIInput::Axis::Y, (b.yAxis != -1 ? input->getAxisRepeat(b.yAxis) : 0) + (b.yAxisAlt != -1 ? input->getAxisRepeat(b.yAxisAlt) : 0));
	}
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
			curFocus->setFocused(false);
		}

		currentFocus = focus;
		if (focus) {
			focus->setFocused(true);
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
	if (!start->isActive()) {
		return {};
	}

	// Depth first
	for (auto& c: start->getChildren()) {
		auto result = getWidgetUnderMouse(c, mousePos);
		if (result) {
			return result;
		}
	}

	auto rect = start->getMouseRect();
	if (start->canInteractWithMouse() && rect.contains(mousePos)) {
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

	if (start->canInteractWithMouse()) {
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
		audio->play(clip, AudioPosition::makeUI(0.0f), 1.0f, false, Random::getGlobal().getFloat(0.95f, 1.05f));
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
