#include "ui/ui_root.h"
#include "ui/ui_widget.h"
#include "graphics/sprite/sprite_painter.h"
#include "api/audio_api.h"

using namespace Halley;

void UIParent::addChild(std::shared_ptr<UIWidget> widget)
{
	widget->setParent(*this);
	children.push_back(widget);
}

void UIParent::removeChild(UIWidget& widget)
{
	children.erase(std::remove_if(children.begin(), children.end(), [&] (auto& c)
	{
		return c.get() == &widget;
	}), children.end());
}

std::vector<std::shared_ptr<UIWidget>>& UIParent::getChildren()
{
	return children;
}

const std::vector<std::shared_ptr<UIWidget>>& UIParent::getChildren() const
{
	return children;
}

UIPainter::UIPainter(SpritePainter& painter, int mask, int layer)
	: painter(painter)
	, mask(mask)
	, layer(layer)
	, n(0)
{
}

void UIPainter::draw(const Sprite& sprite)
{
	painter.add(sprite, mask, layer, float(n++));
}

void UIPainter::draw(const TextRenderer& sprite)
{
	painter.add(sprite, mask, layer, float(n++));
}

UIRoot* UIRoot::getRoot()
{
	return this;
}

UIRoot::UIRoot(AudioAPI* audio)
	: audio(audio)
{
}

void UIRoot::update(Time t, spInputDevice mouse, spInputDevice manual, Vector2f uiOffset)
{
	// Layout all widgets
	for (auto& c: getChildren()) {
		c->layout();
	}

	updateManual(manual);
	updateMouse(mouse, uiOffset);

	// Update children
	for (auto& c: getChildren()) {
		c->doUpdate(t);
	}

	// Layout again, since update might have caused items to be spawned
	for (auto& c: getChildren()) {
		c->layout();
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
			underMouse->pressMouse(0);
		}
	}

	// Release click
	auto focus = currentFocus.lock();
	if (mouse->isButtonReleased(0)) {
		mouseHeld = false;
		if (focus) {
			focus->releaseMouse(0);
		}
	}

	// If the mouse is held, but it's over a different component from the focused one, don't mouse over anything
	auto activeMouseOver = underMouse;
	if (mouseHeld && focus && focus != underMouse) {
		activeMouseOver.reset();
	}
	updateMouseOver(activeMouseOver);
}

void UIRoot::updateManual(spInputDevice manual)
{
	int x = manual->getAxisRepeat(0);
	int y = manual->getAxisRepeat(1);
	if (x > 0 || y > 0) {
		mouseOverNext(true);
	} else if (x < 0 || y < 0) {
		mouseOverNext(false);
	}
}

void UIRoot::mouseOverNext(bool forward)
{
	if (getChildren().empty()) {
		return;
	}
	std::vector<std::shared_ptr<UIWidget>> widgets;
	collectWidgets(getChildren().back(), widgets);

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

void UIRoot::setFocus(std::shared_ptr<UIWidget> focus)
{
	auto curFocus = currentFocus.lock();
	if (curFocus) {
		curFocus->setFocused(false);
	}

	currentFocus = focus;
	if (focus) {
		focus->setFocused(true);
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
	if (getChildren().empty()) {
		return {};
	}
	return getWidgetUnderMouse(getChildren().back(), mousePos);
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

	auto rect = Rect4f(start->getPosition(), start->getPosition() + start->getSize());
	if (start->isFocusable() && rect.isInside(mousePos)) {
		return start;
	} else {
		return {};
	}
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
	return !getChildren().empty();
}
