#include "ui_widget.h"
#include "ui_root.h"
#include "ui_validator.h"
#include "ui_data_bind.h"
#include "ui_anchor.h"
#include "ui_behaviour.h"

using namespace Halley;

UIWidget::UIWidget(String id, Vector2f minSize, Maybe<UISizer> sizer, Vector4f innerBorder)
	: id(id)
	, size(minSize)
	, minSize(minSize)
	, innerBorder(innerBorder)
	, sizer(std::move(sizer))
{
	if (this->sizer) {
		this->sizer.get().reparent(*this);
	}
}

UIWidget::~UIWidget()
{
	if (dataBind) {
		dataBind->setWidget(nullptr);
		dataBind.reset();
	}
}

void UIWidget::doDraw(UIPainter& painter) const
{
	if (isActive()) {
		draw(painter);

		if (childLayerAdjustment == 0) {
			drawChildren(painter);
		} else {
			UIPainter p2 = painter.withAdjustedLayer(childLayerAdjustment);
			drawChildren(p2);
		}

		drawAfterChildren(painter);
	}
}

void UIWidget::doUpdate(UIWidgetUpdateType updateType, Time t, UIInputType inputType, JoystickType joystickType)
{
	if (updateType == Full || updateType == First) {
		setInputType(inputType);
		setJoystickType(joystickType);
	
		if (validator) {
			setEnabled(getValidator()->isEnabled());
		}

		checkActive();
	}

	if (isActive()) {
		updateBehaviours(t);
		update(t, positionUpdated);
		positionUpdated = false;

		if (inputButtons && updateType == First) {
			onInput(inputResults, t);
		}

		addNewChildren(inputType);

		for (auto& c: getChildren()) {
			c->doUpdate(updateType, t, inputType, joystickType);
		}

		removeDeadChildren();

		if (eventHandler) {
			eventHandler->pump();
		}
	}
}

Vector2f UIWidget::getLayoutMinimumSize(bool force) const
{
	if (!isActive() && !force) {
		return {};
	}
	Vector2f minSize = getMinimumSize();

	if (sizer) {
		if (layoutNeeded > 0) {
			--layoutNeeded;
			auto border = getInnerBorder();
			layoutSize = sizer.get().getLayoutMinimumSize(false);
			if (layoutSize.x > 0.1f || layoutSize.y > 0.1f) {
				layoutSize += Vector2f(border.x + border.z, border.y + border.w);
			}
		}
		return Vector2f::max(minSize, layoutSize);
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
	checkActive();
	Vector2f minimumSize = getLayoutMinimumSize(false);
	Vector2f targetSize = Vector2f::max(shrinkOnLayout ? Vector2f() : size, minimumSize);
	setRect(Rect4f(getPosition(), getPosition() + targetSize));

	alignAtAnchor();
	onLayout();
}

void UIWidget::alignAt(const UIAnchor& a)
{
	if (parent) {
		a.position(*this);
	}
}

void UIWidget::alignAtAnchor()
{
	if (anchor) {
		alignAt(*anchor);
	}
}

void UIWidget::setAnchor(UIAnchor a)
{
	if (!anchor || a != *anchor) {
		anchor = std::make_unique<UIAnchor>(std::move(a));
		if (parent) {
			layout();
		}
	}
}

void UIWidget::setAnchor()
{
	anchor.reset();
}

Maybe<UISizer>& UIWidget::tryGetSizer()
{
	return sizer;
}

UISizer& UIWidget::getSizer()
{
	if (!sizer) {
		throw Exception("UIWidget does not have a sizer.", HalleyExceptions::UI);
	}
	return sizer.get();
}

void UIWidget::add(std::shared_ptr<IUIElement> element, float porportion, Vector4f border, int fillFlags)
{
	auto widget = std::dynamic_pointer_cast<UIWidget>(element);
	if (widget) {
		addChild(widget);
	} else {
		auto s = std::dynamic_pointer_cast<UISizer>(element);
		if (s) {
			s->reparent(*this);
		}
	}
	if (sizer) {
		sizer.get().add(element, porportion, border, fillFlags);
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

void UIWidget::clear()
{
	if (sizer) {
		sizer.get().clear();
	}
	UIParent::clear();
}

bool UIWidget::canInteractWithMouse() const
{
	return false;
}

bool UIWidget::isFocused() const
{
	return focused;
}

void UIWidget::setId(const String& i)
{
	id = i;
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

Rect4f UIWidget::getRect() const
{
	return Rect4f(getPosition(), getPosition() + getSize());
}

void UIWidget::setPosition(Vector2f pos)
{
	position = pos;
	positionUpdated = true;
}

void UIWidget::setMinSize(Vector2f size)
{
	if (minSize != size) {
		minSize = size;
		markAsNeedingLayout();
	}
}

void UIWidget::setInnerBorder(Vector4f border)
{
	if (innerBorder != border) {
		innerBorder = border;
		markAsNeedingLayout();
	}
}

void UIWidget::setFocused(bool f)
{
	if (focused != f) {
		focused = f;
		if (focused) {
			onFocus();
			sendEvent(UIEvent(UIEventType::FocusGained, getId()));
		} else {
			onFocusLost();
			sendEvent(UIEvent(UIEventType::FocusLost, getId()));
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

void UIWidget::onMouseOver(Vector2f mousePos)
{
}

bool UIWidget::isActive() const
{
	return activeByUser && activeByInput;
}

void UIWidget::setActive(bool s)
{
	const bool wasActive = isActive();
	activeByUser = s;
	updateActive(wasActive);
}

void UIWidget::updateActive(bool wasActiveBefore)
{
	if (isActive() != wasActiveBefore) {
		if (!isActive()) {
			resetInputResults();
		}

		markAsNeedingLayout();
	}
}

bool UIWidget::isEnabled() const
{
	return enabled;
}

void UIWidget::setEnabled(bool e)
{
	if (enabled != e) {
		enabled = e;
		if (!enabled) {
			resetInputResults();
		}

		markAsNeedingLayout();
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

const UIRoot* UIWidget::getRoot() const
{
	return parent ? parent->getRoot() : nullptr;
}

void UIWidget::notifyDataBind(bool data) const
{
	if (dataBind) {
		dataBind->onDataFromWidget(data);
	}
}

void UIWidget::notifyDataBind(int data) const
{
	if (dataBind) {
		dataBind->onDataFromWidget(data);
	}
}

void UIWidget::notifyDataBind(float data) const
{
	if (dataBind) {
		dataBind->onDataFromWidget(data);
	}
}

void UIWidget::notifyDataBind(const String& data) const
{
	if (dataBind) {
		dataBind->onDataFromWidget(data);
	}
}

void UIWidget::shrink()
{
	if (size.x > minSize.x + 0.5f || size.y > minSize.y + 0.5f) {
		auto prevShrink = shrinkOnLayout;
		shrinkOnLayout = true;
		layout();
		shrinkOnLayout = prevShrink;
	}
}

void UIWidget::forceLayout()
{
	Expects (lastInputType != UIInputType::Undefined);
	forceAddChildren(lastInputType);
	layout();
}

void UIWidget::forceAddChildren(UIInputType inputType)
{
	addNewChildren(inputType);
	for (auto& c: getChildren()) {
		c->forceAddChildren(inputType);
	}
	checkActive();
}

void UIWidget::addBehaviour(std::shared_ptr<UIBehaviour> behaviour)
{
	behaviours.push_back(std::move(behaviour));
	behaviours.back()->doInit(*this);
}

void UIWidget::clearBehaviours()
{
	for (auto& b: behaviours) {
		b->doDeInit();
	}
	behaviours.clear();
}

const std::vector<std::shared_ptr<UIBehaviour>>& UIWidget::getBehaviours() const
{
	return behaviours;
}

UIInputType UIWidget::getLastInputType() const
{
	return lastInputType;
}

void UIWidget::onInput(const UIInputResults& input, Time time)
{
}

void UIWidget::setMouseClip(Maybe<Rect4f> clip)
{
	mouseClip = clip;
	for (auto& c: getChildren()) {
		c->setMouseClip(clip);
	}
}

void UIWidget::onManualControlCycleValue(int delta)
{
}

void UIWidget::onManualControlAnalogueAdjustValue(float delta, Time t)
{
}

void UIWidget::onManualControlActivate()
{
}

void UIWidget::onEnabledChanged()
{
}

void UIWidget::onParentChanged()
{
}

void UIWidget::setParent(UIParent* p)
{
	Expects((parent == nullptr) ^ (p == nullptr));
	parent = p;

	if (parent) {
		parent->markAsNeedingLayout();
	}

	if (anchor) {
		layout();
	}

	onParentChanged();
}

UIParent* UIWidget::getParent() const
{
	return parent;
}

void UIWidget::destroy()
{
	if (alive && !destroying) {
		destroying = true;
		onDestroyRequested();
		bool ok = true;
		for (auto& b: behaviours) {
			ok = ok && b->onParentDestroyed();
		}
		if (ok) {
			forceDestroy();
		}
	}
}

void UIWidget::forceDestroy()
{
	destroying = true;
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
	handler->setWidget(this);
}

UIEventHandler& UIWidget::getEventHandler()
{
	if (!eventHandler) {
		setEventHandler(std::make_shared<UIEventHandler>());
	}
	return *eventHandler;
}

void UIWidget::setHandle(UIEventType type, UIEventCallback handler)
{
	getEventHandler().setHandle(type, std::move(handler));
}

void UIWidget::setHandle(UIEventType type, String id, UIEventCallback handler)
{
	getEventHandler().setHandle(type, std::move(id), std::move(handler));
}

void UIWidget::setCanSendEvents(bool canSend)
{
	canSendEvents = canSend;
}

void UIWidget::setInputType(UIInputType uiInput)
{
	lastInputType = uiInput;
	if (!onlyEnabledWithInputs.empty()) {
		const bool wasActive = isActive();
		activeByInput = std::find(onlyEnabledWithInputs.begin(), onlyEnabledWithInputs.end(), uiInput) != onlyEnabledWithInputs.end();
		updateActive(wasActive);
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

void UIWidget::drawAfterChildren(UIPainter& painter) const
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

void UIWidget::updateBehaviours(Time t)
{
	for (auto& behaviour: behaviours) {
		behaviour->update(t);
	}

	auto firstRemoved = std::remove_if(behaviours.begin(), behaviours.end(), [] (const std::shared_ptr<UIBehaviour>& behaviour) { return !behaviour->isAlive(); });
	for (auto iter = firstRemoved; iter != behaviours.end(); ++iter) {
		(*iter)->doDeInit();
	}
	behaviours.erase(firstRemoved, behaviours.end());
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

void UIWidget::onDestroyRequested()
{
}

void UIWidget::sendEvent(UIEvent&& event) const
{
	if (canSendEvents) {
		if (eventHandler && eventHandler->canHandle(event)) {
			eventHandler->queue(event);
		} else {
			if (parent) {
				parent->sendEvent(std::move(event));
			}
		}
	}
}

void UIWidget::sendEventDown(const UIEvent& event) const
{
	if (eventHandler && eventHandler->canHandle(event)) {
		eventHandler->queue(event);
	} else {
		for (auto& c: getChildren()) {
			c->sendEventDown(event);
		}
	}
}

Maybe<AudioHandle> UIWidget::playSound(const String& eventName)
{
	if (!eventName.isEmpty()) {
		auto root = getRoot();
		if (root) {
			return root->playSound(eventName);
		}
	}

	return {};
}

bool UIWidget::needsLayout() const
{
	return layoutNeeded > 0;
}

void UIWidget::markAsNeedingLayout()
{
	layoutNeeded = 1;
	if (parent) {
		parent->markAsNeedingLayout();
	}
	if (sizer) {
		sizer->updateEnabled();
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

void UIWidget::resetInputResults()
{
	inputResults.reset();
	for (auto& c: getChildren()) {
		c->resetInputResults();
	}
}

void UIWidget::setValidator(std::shared_ptr<UIValidator> v)
{
	validator = v;
	if (validator) {
		onValidatorSet();
	}
}

std::shared_ptr<UIValidator> UIWidget::getValidator() const
{
	return validator;
}

void UIWidget::onValidatorSet()
{
}

UIDataBind::Format UIWidget::getDataBindFormat() const
{
	if (dataBind) {
		return dataBind->getFormat();
	} else {
		return UIDataBind::Format::Undefined;
	}
}

void UIWidget::setDataBind(std::shared_ptr<UIDataBind> d)
{
	dataBind = d;
	dataBind->setAcceptingDataFromWidget(false);
	dataBind->setWidget(this);
	readFromDataBind();
	dataBind->setAcceptingDataFromWidget(true);
}

std::shared_ptr<UIDataBind> UIWidget::getDataBind() const
{
	return dataBind;
}

void UIWidget::readFromDataBind()
{
}

void UIWidget::bindData(const String& childId, bool initialValue, UIDataBindBool::WriteCallback callback)
{
	auto widget = tryGetWidget(childId);
	if (widget) {
		widget->setDataBind(std::make_shared<UIDataBindBool>(initialValue, std::move(callback)));
	}
}

void UIWidget::bindData(const String& childId, int initialValue, UIDataBindInt::WriteCallback callback)
{
	auto widget = tryGetWidget(childId);
	if (widget) {
		widget->setDataBind(std::make_shared<UIDataBindInt>(initialValue, std::move(callback)));
	}
}

void UIWidget::bindData(const String& childId, float initialValue, UIDataBindFloat::WriteCallback callback)
{
	auto widget = tryGetWidget(childId);
	if (widget) {
		widget->setDataBind(std::make_shared<UIDataBindFloat>(initialValue, std::move(callback)));
	}
}

void UIWidget::bindData(const String& childId, const String& initialValue, UIDataBindString::WriteCallback callback)
{
	auto widget = tryGetWidget(childId);
	if (widget) {
		widget->setDataBind(std::make_shared<UIDataBindString>(initialValue, std::move(callback)));
	}
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
	if (shrink != shrinksOnLayout()) {
		markAsNeedingLayout();
		shrinkOnLayout = shrink;
	}
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

void UIWidget::setChildLayerAdjustment(int delta)
{
	childLayerAdjustment = delta;
}

int UIWidget::getChildLayerAdjustment() const
{
	return childLayerAdjustment;
}
