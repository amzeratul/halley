#include "ui/ui_widget.h"
#include "ui/ui_root.h"

using namespace Halley;

UIWidget::UIWidget(String id, Vector2f minSize, Maybe<UISizer> sizer, Vector4f innerBorder)
	: id(id)
	, size(minSize)
	, minSize(minSize)
	, innerBorder(innerBorder)
	, sizer(sizer)
{
}

UIWidget::~UIWidget()
{
}

void UIWidget::doDraw(UIPainter& painter) const
{
	if (enabled) {
		draw(painter);
		for (auto& c: getChildren()) {
			c->doDraw(painter);
		}
	}
}

void UIWidget::doUpdate(Time t, UIInputType inputType)
{
	if (enabled) {
		setInputType(inputType);
		update(t, positionUpdated);
		positionUpdated = false;
		for (auto& c: getChildren()) {
			c->doUpdate(t, inputType);
		}
	}
}

Vector2f UIWidget::computeMinimumSize() const
{
	if (!enabled) {
		return {};
	}
	Vector2f minSize = getMinimumSize();
	if (sizer) {
		auto border = getInnerBorder();
		Vector2f innerSize = sizer.get().computeMinimumSize();
		if (innerSize.x > 0.1f || innerSize.y > 0.1f) {
			innerSize += Vector2f(border.x + border.z, border.y + border.w);
		}
		return Vector2f::max(minSize, innerSize);
	}
	return minSize;
}

void UIWidget::setRect(Rect4f rect)
{
	setWidgetRect(rect);
	if (sizer) {
		auto border = getInnerBorder();
		auto p0 = getPosition();
		sizer.get().setRect(Rect4f(p0 + Vector2f(border.x, border.y), p0 + rect.getSize() - Vector2f(border.z, border.w)));
	}
}

void UIWidget::layout(bool shrink)
{
	Vector2f minimumSize = computeMinimumSize();
	Vector2f targetSize = Vector2f::max(shrink ? Vector2f() : size, minimumSize);
	setRect(Rect4f(getPosition(), getPosition() + targetSize));
}

void UIWidget::centerAt(Vector2f pos)
{
	layout();
	setPosition(pos - (size / 2.0f).floor());
}

Maybe<UISizer>& UIWidget::tryGetSizer()
{
	return sizer;
}

UISizer& UIWidget::getSizer()
{
	if (!sizer) {
		throw Exception("UIWidget does not have a sizer.");
	}
	return sizer.get();
}

void UIWidget::add(std::shared_ptr<UIWidget> widget, float porportion, Vector4f border, int fillFlags)
{
	addChild(widget);
	if (sizer) {
		sizer.get().add(widget, porportion, border, fillFlags);
	}
}

void UIWidget::add(std::shared_ptr<UISizer> s, float proportion, Vector4f border, int fillFlags)
{
	s->reparent(*this);
	if (sizer) {
		sizer.get().add(s, proportion, border, fillFlags);
	}
}

void UIWidget::addSpacer(float size)
{
	if (sizer) {
		sizer.get().addSpacer(size);
	}
}

void UIWidget::addStretchSpacer(float proportion)
{
	if (sizer) {
		sizer.get().addStretchSpacer(proportion);
	}
}

bool UIWidget::isFocusable() const
{
	return false;
}

bool UIWidget::isFocused() const
{
	return focused;
}

bool UIWidget::isFocusLocked() const
{
	return false;
}

bool UIWidget::isMouseOver() const
{
	return mouseOver;
}

String UIWidget::getId() const
{
	return id;
}

Vector2f UIWidget::getPosition() const
{
	return position;
}

Vector2f UIWidget::getSize() const
{
	return size;
}

Vector2f UIWidget::getMinimumSize() const
{
	return minSize;
}

Vector4f UIWidget::getInnerBorder() const
{
	return innerBorder;
}

void UIWidget::setPosition(Vector2f pos)
{
	position = pos;
	positionUpdated = true;
}

void UIWidget::setMinSize(Vector2f size)
{
	minSize = size;
}

void UIWidget::setFocused(bool f)
{
	if (focused != f) {
		focused = f;
		if (focused) {
			onFocus();
		} else {
			onFocusLost();
		}
	}
}

void UIWidget::setMouseOver(bool mo)
{
	mouseOver = mo;
}

void UIWidget::pressMouse(int button)
{
}

void UIWidget::releaseMouse(int button)
{
}

bool UIWidget::isEnabled() const
{
	return enabled;
}

void UIWidget::setEnabled(bool e)
{
	enabled = e;
}

UIRoot* UIWidget::getRoot()
{
	return parent ? parent->getRoot() : nullptr;
}

void UIWidget::setParent(UIParent& p)
{
	parent = &p;
}

void UIWidget::destroy()
{
	if (parent) {
		parent->removeChild(*this);
	}
	doDestroy();
}

std::shared_ptr<UIWidget> UIWidget::getWidget(const String& id)
{
	for (auto& c: getChildren()) {
		if (c->getId() == id) {
			return c;
		}
		auto c2 = c->getWidget(id);
		if (c2) {
			return c2;
		}
	}
	return {};
}

void UIWidget::doDestroy()
{
	for (auto& c: getChildren()) {
		c->doDestroy();
	}
}

void UIWidget::setEventHandler(std::shared_ptr<UIEventHandler> handler)
{
	eventHandler = handler;
}

UIEventHandler& UIWidget::getEventHandler()
{
	if (!eventHandler) {
		eventHandler = std::make_shared<UIEventHandler>();
	}
	return *eventHandler;
}

void UIWidget::setInputType(UIInputType uiInput)
{
	enabled = onlyEnabledWithInput == UIInputType::Undefined || uiInput == onlyEnabledWithInput;
}

void UIWidget::setOnlyEnabledWithInput(UIInputType uiInput)
{
	onlyEnabledWithInput = uiInput;
}

UIInputType UIWidget::getOnlyEnabledWithInput() const
{
	return onlyEnabledWithInput;
}

void UIWidget::draw(UIPainter& painter) const
{
}

void UIWidget::update(Time t, bool moved)
{
}

void UIWidget::onFocus()
{
}

void UIWidget::onFocusLost()
{
}

void UIWidget::sendEvent(UIEvent&& event) const
{
	if (!eventHandler || !eventHandler->handle(event)) {
		parent->sendEvent(std::move(event));
	}
}

void UIWidget::playSound(const std::shared_ptr<const AudioClip>& clip)
{
	if (clip) {
		auto root = getRoot();
		if (root) {
			root->playSound(clip);
		}
	}
}

void UIWidget::setWidgetRect(Rect4f rect)
{
	if (position != rect.getTopLeft()) {
		position = rect.getTopLeft();
		positionUpdated = true;
	}
	if (size != rect.getSize()) {
		size = rect.getSize();
		positionUpdated = true;
	}
}
