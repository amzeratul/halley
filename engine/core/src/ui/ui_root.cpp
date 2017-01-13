#include "ui/ui_root.h"
#include "ui/ui_widget.h"
#include "graphics/sprite/sprite_painter.h"
#include "api/audio_api.h"

using namespace Halley;

void UIParent::addChild(std::shared_ptr<UIWidget> widget)
{
	widget->setParent(*this);
	getRoot().addWidget(*widget);
	children.push_back(widget);
}

void UIParent::removeChild(UIWidget& widget)
{
	children.erase(std::remove_if(children.begin(), children.end(), [&] (auto& c)
	{
		return c.get() == &widget;
	}), children.end());
	getRoot().removeWidget(widget);
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

UIRoot& UIRoot::getRoot()
{
	return *this;
}

UIRoot::UIRoot(AudioAPI* audio)
	: audio(audio)
{
}

void UIRoot::addWidget(UIWidget& widget)
{
	widgets.push_back(&widget);
}

void UIRoot::removeWidget(UIWidget& widget)
{
	widgets.erase(std::remove(widgets.begin(), widgets.end(), &widget), widgets.end());
}

void UIRoot::update(Time t, Vector2f mousePos, bool mousePressed, bool mouseReleased)
{
	std::shared_ptr<UIWidget> underMouse;

	for (auto& c: getChildren()) {
		c->layout();
		underMouse = getWidgetUnderMouse(c, mousePos);
	}

	if (mousePressed) {
		setFocus(underMouse);
		if (underMouse) {
			underMouse->pressMouse(0);
		}
	}

	auto focus = currentFocus.lock();
	if (mouseReleased) {
		if (focus) {
			focus->releaseMouse(0);
		}
	}

	auto activeMouseOver = underMouse;
	if (focus && focus != underMouse) {
		activeMouseOver.reset();
	}
	updateMouseOver(underMouse);

	for (auto& c: widgets) {
		c->doUpdate(t);
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

