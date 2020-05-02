#include "ui_root.h"
#include "ui_widget.h"
#include "halley/core/api/audio_api.h"
#include "ui_painter.h"
#include "halley/audio/audio_position.h"
#include "halley/audio/audio_clip.h"
#include "halley/core/input/input_keyboard.h"
#include "halley/maths/random.h"

using namespace Halley;

UIRoot::UIRoot(const HalleyAPI& api, Rect4f rect)
	: id("root")
	, audio(api.audio)
	, uiRect(rect)
	, dummyInput(std::make_shared<InputButtonBase>(4))
	, mouseRemap([](Vector2f p) { return p; })
{
	if (api.platform && api.platform->hasKeyboard()) {
		keyboard = api.platform->getKeyboard();
	} else {
		keyboard = api.input->getKeyboard();
	}
}

UIRoot::~UIRoot() = default;

UIRoot* UIRoot::getRoot()
{
	return this;
}

const UIRoot* UIRoot::getRoot() const
{
	return this;
}

const String& UIRoot::getId() const
{
	return id;
}

void UIRoot::setRect(Rect4f rect, Vector2f overscan)
{
	uiRect = Rect4f(rect.getTopLeft() + overscan, rect.getBottomRight() - overscan);
	this->overscan = overscan;
}

Rect4f UIRoot::getRect() const
{
	return uiRect;
}

void UIRoot::update(Time t, UIInputType activeInputType, spInputDevice mouse, spInputDevice manual)
{
	auto joystickType = manual->getJoystickType();
	bool first = true;

	do {
		// Spawn new widgets
		addNewChildren(activeInputType);

		// Update input
		if (activeInputType == UIInputType::Mouse) {
			updateMouse(mouse);
		}
		updateFocusInput();
		updateGlobalInput(manual);

		// Update children
		for (auto& c: getChildren()) {
			c->doUpdate(first ? UIWidgetUpdateType::First : UIWidgetUpdateType::Full, t, activeInputType, joystickType);
		}
		first = false;
		removeDeadChildren();

		// Layout all widgets
		runLayout();

		// Update again, to reflect what happened >_>
		for (auto& c: getChildren()) {
			c->doUpdate(UIWidgetUpdateType::Partial, 0, activeInputType, joystickType);
		}

		// For subsequent iterations, make sure t = 0
		t = 0;
	} while (isWaitingToSpawnChildren());
}

void UIRoot::updateMouse(spInputDevice mouse)
{
	// Check where we should be mouse overing.
	// If the mouse hasn't moved, keep the last one.
	std::shared_ptr<UIWidget> underMouse;
	Vector2f mousePos = mouseRemap(mouse->getPosition() + uiRect.getTopLeft() - overscan);
	if (true || (mousePos - lastMousePos).squaredLength() > 0.01f) {
		// Go through all root-level widgets and find the actual widget under the mouse
		underMouse = getWidgetUnderMouse(mousePos);
		lastMousePos = mousePos;
	} else {
		underMouse = currentMouseOver.lock();
	}

	for (int i = 0; i < 3; ++i) {
		// Click
		if (!anyMouseButtonHeld && mouse->isButtonPressed(i)) {
			anyMouseButtonHeld = true;
			setFocus(underMouse);
			if (underMouse) {
				mouse->clearButtonPress(i);
				underMouse->pressMouse(mousePos, i);
			}
		}

		// Release click
		if (anyMouseButtonHeld && mouse->isButtonReleased(i)) {
			anyMouseButtonHeld = false;
			auto focus = currentFocus.lock();
			if (focus) {
				focus->releaseMouse(mousePos, i);
				mouse->clearButtonRelease(i);
			}
		}
	}

	// Mouse wheel
	int wheelDelta = mouse->getWheelMove();
	if (wheelDelta != 0 && underMouse) {
		underMouse->sendEvent(UIEvent(UIEventType::MouseWheel, "mouse", wheelDelta));
	}

	// If the mouse is held, but it's over a different component from the focused one, don't mouse over anything
	auto activeMouseOver = underMouse;
	if (auto focus = currentFocus.lock(); anyMouseButtonHeld && focus) {
		if (focus != underMouse) {
			activeMouseOver.reset();
		}
		focus->onMouseOver(mousePos);
	} else if (activeMouseOver) {
		activeMouseOver->onMouseOver(mousePos);
	}
	updateMouseOver(activeMouseOver);
}

void UIRoot::updateGlobalInputTree(const spInputDevice& input, UIWidget& widget, std::vector<UIWidget*>& inputTargets, UIInput::Priority& bestPriority, bool accepting)
{
	if (!widget.isActive()) {
		return;
	}

	if (!widget.isEnabled()) {
		accepting = false;
	}

	for (auto& c: widget.getChildren()) {
		// Depth-first
		updateGlobalInputTree(input, *c, inputTargets, bestPriority, accepting);
	}

	if (widget.inputButtons) {
		widget.inputResults.reset();
		if (accepting) {
			auto priority = widget.getInputPriority();

			if (static_cast<int>(priority) > static_cast<int>(bestPriority)) {
				bestPriority = priority;
				inputTargets.clear();
			}
			if (priority == bestPriority) {
				inputTargets.push_back(&widget);
			}
		}
	}
}

void UIRoot::updateGlobalInput(spInputDevice input)
{
	auto& cs = getChildren();
	std::vector<UIWidget*> inputTargets;
	UIInput::Priority bestPriority = UIInput::Priority::Lowest;

	bool accepting = true;
	for (int i = int(cs.size()); --i >= 0; ) {
		auto& c = *cs[i];
		updateGlobalInputTree(input, c, inputTargets, bestPriority, accepting);

		if (c.isMouseBlocker()) {
			accepting = false;
		}
	}

	for (auto& target: inputTargets) {
		auto& b = *target->inputButtons;
		auto& results = target->inputResults;
		results.reset();
		if (b.accept != -1) {
			results.setButton(UIInput::Button::Accept, input->isButtonPressed(b.accept), input->isButtonReleased(b.accept), input->isButtonDown(b.accept));
		}
		if (b.cancel != -1) {
			results.setButton(UIInput::Button::Cancel, input->isButtonPressed(b.cancel), input->isButtonReleased(b.cancel), input->isButtonDown(b.cancel));
		}
		if (b.prev != -1) {
			results.setButton(UIInput::Button::Prev, input->isButtonPressed(b.prev), input->isButtonReleased(b.prev), input->isButtonDown(b.prev));
		}
		if (b.next != -1) {
			results.setButton(UIInput::Button::Next, input->isButtonPressed(b.next), input->isButtonReleased(b.next), input->isButtonDown(b.next));
		}
		if (b.hold != -1) {
			results.setButton(UIInput::Button::Hold, input->isButtonPressed(b.hold), input->isButtonReleased(b.hold), input->isButtonDown(b.hold));
		}
		results.setAxis(UIInput::Axis::X, (b.xAxis != -1 ? input->getAxis(b.xAxis) : 0) + (b.xAxisAlt != -1 ? input->getAxis(b.xAxisAlt) : 0));
		results.setAxis(UIInput::Axis::Y, (b.yAxis != -1 ? input->getAxis(b.yAxis) : 0) + (b.yAxisAlt != -1 ? input->getAxis(b.yAxisAlt) : 0));
		results.setAxisRepeat(UIInput::Axis::X, (b.xAxis != -1 ? input->getAxisRepeat(b.xAxis) : 0) + (b.xAxisAlt != -1 ? input->getAxisRepeat(b.xAxisAlt) : 0));
		results.setAxisRepeat(UIInput::Axis::Y, (b.yAxis != -1 ? input->getAxisRepeat(b.yAxis) : 0) + (b.yAxisAlt != -1 ? input->getAxisRepeat(b.yAxisAlt) : 0));
	}
}

void UIRoot::updateFocusInput()
{
	// Focus could have been destructed, clean up
	if (currentFocus.expired()) {
		setFocus({});
	}
	
	if (textCapture) {
		const bool stillCaptured = textCapture->update();
		if (!stillCaptured) {
			// This is used for soft keyboards, which will return false once they're done executing
			// The widget then loses focus, so that being focused is equal to capturing soft keyboard
			setFocus({});
		}
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

void UIRoot::setFocus(const std::shared_ptr<UIWidget>& newFocus)
{
	if (currentFocus.expired()) {
		currentFocus.reset();
		textCapture.reset();
	}
	
	const auto prevFocus = currentFocus.lock();

	if (prevFocus != newFocus) {
		if (prevFocus) {
			unfocusWidget(*prevFocus);
		}

		currentFocus = newFocus;

		if (newFocus) {
			focusWidget(*newFocus);
		}
	}
}

void UIRoot::focusWidget(UIWidget& widget)
{
	if (!widget.focused) {
		widget.focused = true;
		widget.onFocus();

		const auto text = widget.getTextInputData();
		if (text && keyboard) {
			textCapture = std::make_unique<TextInputCapture>(keyboard->captureText(*text, {}));
		}
		
		widget.sendEvent(UIEvent(UIEventType::FocusGained, widget.getId()));
	}
}

void UIRoot::unfocusWidget(UIWidget& widget)
{
	textCapture.reset();
	
	if (widget.focused) {
		widget.focused = false;
		widget.onFocusLost();
		widget.sendEvent(UIEvent(UIEventType::FocusLost, widget.getId()));
	}
}

void UIRoot::focusNext(bool reverse)
{
	std::vector<std::shared_ptr<UIWidget>> focusables;
	descend([&] (const std::shared_ptr<UIWidget>& e)
	{
		if (e->canReceiveFocus()) {
			focusables.push_back(e);
		}
	});

	if (focusables.empty()) {
		return;
	}

	const int index = gsl::narrow<int>(std::find(focusables.begin(), focusables.end(), currentFocus.lock()) - focusables.begin());
	const int newIndex = modulo(index + (reverse ? -1 : 1), gsl::narrow<int>(focusables.size()));
	setFocus(focusables[newIndex]);
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


std::shared_ptr<UIWidget> UIRoot::getWidgetUnderMouse(Vector2f mousePos, bool includeDisabled) const
{
	auto& cs = getChildren();
	for (int i = int(cs.size()); --i >= 0; ) {
		auto& curRootWidget = cs[i];
		auto widget = getWidgetUnderMouse(curRootWidget, mousePos, includeDisabled);
		if (widget) {
			return widget;
		} else {
			if (curRootWidget->isMouseBlocker()) {
				return {};
			}
		}
	}
	return {};
}

std::shared_ptr<UIWidget> UIRoot::getWidgetUnderMouse(const std::shared_ptr<UIWidget>& start, Vector2f mousePos, bool includeDisabled) const
{
	if (!start->isActive() || (!includeDisabled && !start->isEnabled())) {
		return {};
	}

	// Depth first
	for (auto& c: start->getChildren()) {
		auto result = getWidgetUnderMouse(c, mousePos, includeDisabled);
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

void UIRoot::setUIMouseRemapping(std::function<Vector2f(Vector2f)> remapFunction) {
	Expects(remapFunction != nullptr);
	mouseRemap = remapFunction;
}

void UIRoot::unsetUIMouseRemapping() {
	mouseRemap = [](Vector2f p) { return p; };
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

void UIRoot::onChildAdded(UIWidget& child)
{
	child.onAddedToRoot();
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

void UIRoot::render(RenderContext& rc)
{
	for (auto& c: getChildren()) {
		c->doRender(rc);
	}
}

std::optional<AudioHandle> UIRoot::playSound(const String& eventName)
{
	if (audio && !eventName.isEmpty()) {
		return audio->postEvent(eventName, AudioPosition::makeUI(0.0f));
	}

	return {};
}

void UIRoot::sendEvent(UIEvent) const
{
	// Unhandled event
}

bool UIRoot::hasModalUI() const
{
	for (auto& c: getChildren()) {
		if (c->isActive() && c->isModal()) {
			return true;
		}
	}
	return false;
}

bool UIRoot::isMouseOverUI() const
{
	return static_cast<bool>(currentMouseOver.lock());
}

std::shared_ptr<UIWidget> UIRoot::getWidgetUnderMouse() const
{
	return currentMouseOver.lock();
}

std::shared_ptr<UIWidget> UIRoot::getWidgetUnderMouseIncludingDisabled() const
{
	return getWidgetUnderMouse(lastMousePos, true);
}

UIWidget* UIRoot::getCurrentFocus() const
{
	return currentFocus.lock().get();
}
