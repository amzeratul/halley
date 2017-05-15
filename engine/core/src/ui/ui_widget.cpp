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
	if (shown) {
		draw(painter);
		for (auto& c: getChildren()) {
			c->doDraw(painter);
		}
	}
}

void UIWidget::updateInputDevice(InputDevice& device)
{
}

void UIWidget::doUpdate(Time t, UIInputType inputType, InputDevice& inputDevice)
{
	setInputType(inputType);
	if (shown) {
		update(t, positionUpdated);
		positionUpdated = false;
		for (auto& c: getChildren()) {
			c->doUpdate(t, inputType, inputDevice);
		}
		updateInputDevice(inputDevice);
	}
}

Vector2f UIWidget::computeMinimumSize() const
{
	if (!shown) {
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

void UIWidget::pressMouse(Vector2f mousePos, int button)
{
}

void UIWidget::releaseMouse(Vector2f mousePos, int button)
{
}

bool UIWidget::isShown() const
{
	return shown;
}

void UIWidget::setShown(bool e)
{
	shown = e;
}

bool UIWidget::isAlive() const
{
	return alive;
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
	alive = false;
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
	shown = onlyEnabledWithInputs.empty() || std::find(onlyEnabledWithInputs.begin(), onlyEnabledWithInputs.end(), uiInput) != onlyEnabledWithInputs.end();
}

void UIWidget::setOnlyEnabledWithInputs(const std::vector<UIInputType>& uiInput)
{
	onlyEnabledWithInputs = uiInput;
}

const std::vector<UIInputType>& UIWidget::getOnlyEnabledWithInput() const
{
	return onlyEnabledWithInputs;
}

Rect4f UIWidget::getMouseRect() const
{
	return Rect4f(getPosition(), getPosition() + getSize());
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
