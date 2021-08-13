#include "ui_root.h"
#include "ui_widget.h"
#include "halley/core/api/audio_api.h"
#include "ui_painter.h"
#include "halley/audio/audio_position.h"
#include "halley/audio/audio_clip.h"
#include "halley/core/api/halley_api.h"
#include "halley/core/input/input_keyboard.h"
#include "halley/core/input/input_virtual.h"
#include "halley/maths/random.h"
#include "halley/support/logger.h"
#include "widgets/ui_tooltip.h"

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

UIRoot::~UIRoot()
{
	Logger::logInfo(toString(keyPressListeners.size()) + " key press listeners left.");
	keyPressListeners.clear();
	UIParent::clear();
}

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

void UIRoot::runLayout()
{
	for (auto& c: getChildren()) {
		c->layout();
	}
}

void UIRoot::update(Time t, UIInputType activeInputType, spInputDevice mouse, spInputDevice manual)
{
	auto joystickType = manual ? manual->getJoystickType() : JoystickType::Generic;
	bool first = true;

	updateKeyboardInput();

	do {
		// Spawn new widgets
		addNewChildren(activeInputType);

		// Update input
		if (activeInputType == UIInputType::Mouse) {
			updateMouse(mouse);
		}
		updateGamepadInput(manual);

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

void UIRoot::updateGamepadInputTree(const spInputDevice& input, UIWidget& widget, std::vector<UIWidget*>& inputTargets, UIGamepadInput::Priority& bestPriority, bool accepting)
{
	if (!widget.isActive()) {
		return;
	}

	if (!widget.isEnabled()) {
		accepting = false;
	}

	for (auto& c: widget.getChildren()) {
		// Depth-first
		updateGamepadInputTree(input, *c, inputTargets, bestPriority, accepting);
	}

	if (widget.gamepadInputButtons) {
		widget.gamepadInputResults.reset();
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

void UIRoot::updateGamepadInput(const spInputDevice& input)
{
	if (!input) {
		return;
	}
	
	auto& cs = getChildren();
	std::vector<UIWidget*> inputTargets;
	UIGamepadInput::Priority bestPriority = UIGamepadInput::Priority::Lowest;

	bool accepting = true;
	for (int i = int(cs.size()); --i >= 0; ) {
		auto& c = *cs[i];
		updateGamepadInputTree(input, c, inputTargets, bestPriority, accepting);

		if (c.isMouseBlocker() && c.isActive()) {
			accepting = false;
		}
	}

	for (auto& target: inputTargets) {
		auto& b = *target->gamepadInputButtons;
		auto& results = target->gamepadInputResults;
		results.reset();
		if (b.accept != -1) {
			results.setButton(UIGamepadInput::Button::Accept, input->isButtonPressed(b.accept), input->isButtonReleased(b.accept), input->isButtonDown(b.accept));
		}
		if (b.cancel != -1) {
			results.setButton(UIGamepadInput::Button::Cancel, input->isButtonPressed(b.cancel), input->isButtonReleased(b.cancel), input->isButtonDown(b.cancel));
		}
		if (b.prev != -1) {
			results.setButton(UIGamepadInput::Button::Prev, input->isButtonPressed(b.prev), input->isButtonReleased(b.prev), input->isButtonDown(b.prev));
		}
		if (b.next != -1) {
			results.setButton(UIGamepadInput::Button::Next, input->isButtonPressed(b.next), input->isButtonReleased(b.next), input->isButtonDown(b.next));
		}
		if (b.hold != -1) {
			results.setButton(UIGamepadInput::Button::Hold, input->isButtonPressed(b.hold), input->isButtonReleased(b.hold), input->isButtonDown(b.hold));
		}
		if (b.secondary != -1) {
			results.setButton(UIGamepadInput::Button::Secondary, input->isButtonPressed(b.secondary), input->isButtonReleased(b.secondary), input->isButtonDown(b.secondary));
		}
		if (b.tertiary != -1) {
			results.setButton(UIGamepadInput::Button::Tertiary, input->isButtonPressed(b.tertiary), input->isButtonReleased(b.tertiary), input->isButtonDown(b.tertiary));
		}
		results.setAxis(UIGamepadInput::Axis::X, (b.xAxis != -1 ? input->getAxis(b.xAxis) : 0) + (b.xAxisAlt != -1 ? input->getAxis(b.xAxisAlt) : 0));
		results.setAxis(UIGamepadInput::Axis::Y, (b.yAxis != -1 ? input->getAxis(b.yAxis) : 0) + (b.yAxisAlt != -1 ? input->getAxis(b.yAxisAlt) : 0));
		results.setAxisRepeat(UIGamepadInput::Axis::X, (b.xAxis != -1 ? input->getAxisRepeat(b.xAxis) : 0) + (b.xAxisAlt != -1 ? input->getAxisRepeat(b.xAxisAlt) : 0));
		results.setAxisRepeat(UIGamepadInput::Axis::Y, (b.yAxis != -1 ? input->getAxisRepeat(b.yAxis) : 0) + (b.yAxisAlt != -1 ? input->getAxisRepeat(b.yAxisAlt) : 0));
	}
}

void UIRoot::updateKeyboardInput()
{
	// Focus could have been destructed, clean up
	if (currentFocus.expired()) {
		setFocus({});
	}

	if (keyboard) {
		auto focused = currentFocus.lock();
		if (focused && !focused->isActiveInHierarchy()) {
			focused.reset();
		}

		for (const auto& key: keyboard->getPendingKeys()) {
			// Send to focused first
			if (focused) {
				focused->receiveKeyPress(key);
			} else {
				receiveKeyPress(key);
			}
		}
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

void UIRoot::receiveKeyPress(KeyboardKeyPress key)
{
	// This means that a key press fell through the current focus, try sending directly to a registered listener
	
	// Remove expired
	keyPressListeners.erase(std::remove_if(keyPressListeners.begin(), keyPressListeners.end(), [] (const auto& e) { return e.first.expired(); }), keyPressListeners.end());

	// Finds one listener that can handle it (they are sorted by priority)
	for (auto& listener: keyPressListeners) {
		auto widget = listener.first.lock();
		if (widget && widget->isActiveInHierarchy() && widget->onKeyPress(key)) {
			return;
		}
	}

	// None of the listeners handled it
	onUnhandledKeyPress(key);
}

void UIRoot::onUnhandledKeyPress(KeyboardKeyPress key)
{
	if (unhandledKeyPressListener && unhandledKeyPressListener(key)) {
		return;
	}
	
	if (key.is(KeyCode::Tab)) {
		focusNext(false);
	}
	if (key.is(KeyCode::Tab, KeyMods::Shift)) {
		focusNext(true);
	}
}

void UIRoot::registerKeyPressListener(std::shared_ptr<UIWidget> widget, int priority)
{
	keyPressListeners.emplace_back(widget, priority);
	std::sort(keyPressListeners.begin(), keyPressListeners.end(), [] (const auto& a, const auto& b)
	{
		return a.second > b.second;
	});
}

void UIRoot::removeKeyPressListener(const UIWidget& widget)
{
	keyPressListeners.erase(std::remove_if(keyPressListeners.begin(), keyPressListeners.end(), [&] (const auto& v) -> bool
	{
		const auto sharedPtr = v.first.lock();
		return sharedPtr && &(*sharedPtr) == &widget;
	}), keyPressListeners.end());
}

void UIRoot::setUnhandledKeyPressListener(std::function<bool(KeyboardKeyPress)> handler)
{
	unhandledKeyPressListener = std::move(handler);
}

void UIRoot::makeToolTip(const UIStyle& style)
{
	toolTip = std::make_shared<UIToolTip>(style);
	toolTip->setActive(false);
	addChild(toolTip);
}

Vector2f UIRoot::getLastMousePos() const
{
	return lastMousePos;
}

void UIRoot::updateMouse(const spInputDevice& mouse)
{
	// Go through all root-level widgets and find the actual widget under the mouse
	const Vector2f mousePos = mouseRemap(mouse->getPosition() + uiRect.getTopLeft() - overscan);
	const auto exclusive = mouseExclusive.lock();
	const auto actuallyUnderMouse = getWidgetUnderMouse(mousePos);
	lastMousePos = mousePos;

	// Check buttons
	for (int i = 0; i < 3; ++i) {
		// Click
		if (!anyMouseButtonHeld && mouse->isButtonPressed(i)) {
			
			anyMouseButtonHeld = true;
			mouseExclusive = actuallyUnderMouse;

			if (actuallyUnderMouse) {
				if (actuallyUnderMouse->canReceiveFocus()) {
					setFocus(actuallyUnderMouse);
				}
				mouse->clearButtonPress(i);
				actuallyUnderMouse->pressMouse(mousePos, i);
				
				UIEventType pressEvent;
				if (i == 0) pressEvent = UIEventType::MousePressLeft;
				else if (i == 1) pressEvent = UIEventType::MousePressMiddle;
				else pressEvent = UIEventType::MousePressRight;
				
				actuallyUnderMouse->sendEvent(UIEvent(pressEvent, "mouse", mousePos));
			} else {
				const auto& cs = getChildren();
				if (!cs.empty()) {
					UIEventType unhandledPressEVent;
					if (i == 0) unhandledPressEVent = UIEventType::UnhandledMousePressLeft;
					else if (i == 1) unhandledPressEVent = UIEventType::UnhandledMousePressMiddle;
					else unhandledPressEVent = UIEventType::UnhandledMousePressRight;
					cs.back()->sendEvent(UIEvent(unhandledPressEVent, "mouse", mousePos));
				}
				setFocus({});
			}
		}

		// Release click
		if (anyMouseButtonHeld && mouse->isButtonReleased(i)) {
			anyMouseButtonHeld = false;
			
			if (exclusive) {
				exclusive->releaseMouse(mousePos, i);
				mouse->clearButtonRelease(i);

				UIEventType releaseEvent;
				if (i == 0) releaseEvent = UIEventType::MouseReleaseLeft;
				else if (i == 1) releaseEvent = UIEventType::MouseReleaseMiddle;
				else releaseEvent = UIEventType::MouseReleaseRight;

				exclusive->sendEvent(UIEvent(releaseEvent, "mouse", mousePos));
			}
			else {
				const auto& cs = getChildren();
				if (!cs.empty()) {
					UIEventType unhandledReleaseEvent;
					if (i == 0) unhandledReleaseEvent = UIEventType::UnhandledMouseReleaseLeft;
					else if (i == 1) unhandledReleaseEvent = UIEventType::UnhandledMouseReleaseMiddle;
					else unhandledReleaseEvent = UIEventType::UnhandledMouseReleaseRight;
					cs.back()->sendEvent(UIEvent(unhandledReleaseEvent, "mouse", mousePos));
				}
			}

			mouseExclusive.reset();
		}
	}

	// Mouse wheel
	const int wheelDelta = mouse->getWheelMove();
	const std::shared_ptr<UIWidget> exclusiveUnderMouse = !exclusive || exclusive == actuallyUnderMouse ? actuallyUnderMouse : std::shared_ptr<UIWidget>();
	if (wheelDelta != 0 && exclusiveUnderMouse) {
		exclusiveUnderMouse->sendEvent(UIEvent(UIEventType::MouseWheel, "mouse", wheelDelta));
	}

	// Mouse position
	const std::shared_ptr<UIWidget> mousePosTarget = exclusive ? exclusive : actuallyUnderMouse;
	if (mousePosTarget) {
		mousePosTarget->onMouseOver(mousePos);
		if (toolTip) {
			toolTip->showToolTipForWidget(*mousePosTarget, mousePos);
		}
	} else {
		if (toolTip) {
			toolTip->hide();
		}
	}
	
	updateMouseOver(exclusiveUnderMouse);
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
	const auto& cs = getChildren();
	for (int i = static_cast<int>(cs.size()); --i >= 0; ) {
		const auto& curRootWidget = cs[i];
		const auto result = getWidgetUnderMouse(curRootWidget, mousePos, includeDisabled);
		if (result.first) {
			return result.first;
		} else {
			if (curRootWidget->isMouseBlocker() && curRootWidget->isActiveInHierarchy()) {
				return {};
			}
		}
	}
	return {};
}

std::pair<std::shared_ptr<UIWidget>, int> UIRoot::getWidgetUnderMouse(const std::shared_ptr<UIWidget>& curWidget, Vector2f mousePos, bool includeDisabled, int childLayerAdjustment) const
{
	if (!curWidget->isActive() || (!includeDisabled && !curWidget->isEnabled())) {
		return {};
	}

	// Depth first
	const int adjustmentForChildren = childLayerAdjustment + curWidget->getChildLayerAdjustment();
	std::pair<std::shared_ptr<UIWidget>, int> bestResult;
	for (auto& c: curWidget->getChildren()) {
		auto result = getWidgetUnderMouse(c, mousePos, includeDisabled, adjustmentForChildren);
		if (result.first && (!bestResult.first || result.second > bestResult.second)) {
			bestResult = result;
		}
	}
	if (bestResult.first) {
		return bestResult;
	}

	auto rect = curWidget->getMouseRect();
	if (curWidget->canInteractWithMouse() && rect.contains(mousePos)) {
		return { curWidget, childLayerAdjustment };
	} else {
		return {};
	}
}

void UIRoot::setUIMouseRemapping(std::function<Vector2f(Vector2f)> remapFunction)
{
	Expects(remapFunction != nullptr);
	mouseRemap = std::move(remapFunction);
}

void UIRoot::unsetUIMouseRemapping()
{
	mouseRemap = [](Vector2f p) { return p; };
}

std::shared_ptr<UIWidget> UIRoot::getWidgetUnderMouse() const
{
	return currentMouseOver.lock();
}

std::shared_ptr<UIWidget> UIRoot::getWidgetUnderMouseIncludingDisabled() const
{
	return getWidgetUnderMouse(lastMousePos, true);
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

void UIRoot::onWidgetRemoved(const UIWidget& widget)
{
	auto focus = currentFocus.lock();
	if (focus && focus.get() == &widget) {
		currentFocus.reset();
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

void UIRoot::onChildAdded(UIWidget& child)
{
	//child.notifyTreeAddedToRoot(*this);
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
	for (const auto& c: getChildren()) {
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

UIWidget* UIRoot::getCurrentFocus() const
{
	return currentFocus.lock().get();
}
