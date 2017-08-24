#include "ui/ui_widget.h"
#include "ui/ui_root.h"
#include "ui/ui_validator.h"

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
	if (active) {
		draw(painter);
		drawChildren(painter);
	}
}

void UIWidget::doUpdate(bool full, Time t, UIInputType inputType, JoystickType joystickType)
{
	if (full) {
		setInputType(inputType);
		setJoystickType(joystickType);
	
		if (validator) {
			setEnabled(getValidator()->isEnabled());
		}

		checkActive();
	}

	if (active) {
		update(t, positionUpdated);
		positionUpdated = false;
		for (auto& c: getChildren()) {
			c->doUpdate(full, t, inputType, joystickType);
		}
		
		if (inputButtons && full) {
			onInput(inputResults);
		}

		if (eventHandler) {
			eventHandler->pump();
		}
	}
}

Vector2f UIWidget::getLayoutMinimumSize() const
{
	if (!active) {
		return {};
	}
	Vector2f minSize = getMinimumSize();
	if (sizer) {
		auto border = getInnerBorder();
		Vector2f innerSize = sizer.get().getLayoutMinimumSize();
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
		auto p0 = getLayoutOriginPosition();
		sizer.get().setRect(Rect4f(p0 + Vector2f(border.x, border.y), p0 + rect.getSize() - Vector2f(border.z, border.w)));
	} else {
		for (auto& c: getChildren()) {
			c->layout();
		}
	}
}

void UIWidget::layout()
{
	Vector2f minimumSize = getLayoutMinimumSize();
	Vector2f targetSize = Vector2f::max(shrinkOnLayout ? Vector2f() : size, minimumSize);
	setRect(Rect4f(getPosition(), getPosition() + targetSize));
	onLayout();
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

bool UIWidget::canInteractWithMouse() const
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

const String& UIWidget::getId() const
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

void UIWidget::setInnerBorder(Vector4f border)
{
	innerBorder = border;
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

		for (auto& c: getChildren()) {
			c->setFocused(f);
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

bool UIWidget::isActive() const
{
	return active;
}

void UIWidget::setActive(bool s)
{
	active = s;
}

bool UIWidget::isEnabled() const
{
	return enabled;
}

void UIWidget::setEnabled(bool e)
{
	if (enabled != e) {
		enabled = e;
		onEnabledChanged();
	}
}

bool UIWidget::isAlive() const
{
	return alive;
}

UIRoot* UIWidget::getRoot()
{
	return parent ? parent->getRoot() : nullptr;
}

void UIWidget::onInput(const UIInputResults& input)
{
}

void UIWidget::setMouseClip(Maybe<Rect4f> clip)
{
	mouseClip = clip;
	for (auto& c: getChildren()) {
		c->setMouseClip(clip);
	}
}

void UIWidget::onEnabledChanged()
{
}

void UIWidget::setParent(UIParent& p)
{
	parent = &p;
}

void UIWidget::destroy()
{
	alive = false;
}

bool UIWidget::isDescendentOf(const UIWidget& ancestor) const
{
	if (!parent) {
		return false;
	}

	if (parent == &ancestor) {
		return true;
	}

	return parent->isDescendentOf(ancestor);
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
	if (!onlyEnabledWithInputs.empty()) {
		active = std::find(onlyEnabledWithInputs.begin(), onlyEnabledWithInputs.end(), uiInput) != onlyEnabledWithInputs.end();
	}
}

void UIWidget::setJoystickType(JoystickType joystickType)
{
}

void UIWidget::setOnlyEnabledWithInputs(const std::vector<UIInputType>& uiInput)
{
	onlyEnabledWithInputs = uiInput;
	setInputType(UIInputType::Keyboard);
}

const std::vector<UIInputType>& UIWidget::getOnlyEnabledWithInput() const
{
	return onlyEnabledWithInputs;
}

void UIWidget::setInputButtons(const UIInputButtons& buttons)
{
	inputButtons = std::make_unique<UIInputButtons>(buttons);
}

Rect4f UIWidget::getMouseRect() const
{
	auto rect = Rect4f(getPosition(), getPosition() + getSize());
	if (mouseClip) {
		return rect.intersection(mouseClip.get());
	} else {
		return rect;
	}
}

void UIWidget::draw(UIPainter& painter) const
{
}

void UIWidget::drawChildren(UIPainter& painter) const
{
	for (auto& c: getChildren()) {
		c->doDraw(painter);
	}
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

void UIWidget::onLayout()
{
}

void UIWidget::sendEvent(UIEvent&& event) const
{
	if (eventHandler && eventHandler->canHandle(event)) {
		eventHandler->queue(event);
	} else {
		if (parent) {
			parent->sendEvent(std::move(event));
		}
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

void UIWidget::checkActive()
{
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

void UIWidget::setValidator(std::shared_ptr<UIValidator> v)
{
	validator = v;
}

std::shared_ptr<UIValidator> UIWidget::getValidator() const
{
	return validator;
}

bool UIWidget::isModal() const
{
	return modal;
}

void UIWidget::setModal(bool m)
{
	modal = m;
}

bool UIWidget::isMouseBlocker() const
{
	return mouseBlocker;
}

void UIWidget::setMouseBlocker(bool blocker)
{
	mouseBlocker = blocker;
}

bool UIWidget::shrinksOnLayout() const
{
	return shrinkOnLayout;
}

void UIWidget::setShrinkOnLayout(bool shrink)
{
	shrinkOnLayout = shrink;
}

Vector2f UIWidget::getLayoutOriginPosition() const
{
	return getPosition();
}

void UIWidget::updateInputDevice(const InputDevice& inputDevice)
{
}

UIInput::Priority UIWidget::getInputPriority() const
{
	return focused ? UIInput::Priority::Focused : inputButtons->priorityLevel;
}
