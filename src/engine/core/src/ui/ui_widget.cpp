#include "halley/ui/ui_widget.h"

#include "halley/graphics/render_context.h"
#include "halley/ui/ui_root.h"
#include "halley/ui/ui_validator.h"
#include "halley/ui/ui_data_bind.h"
#include "halley/ui/ui_anchor.h"
#include "halley/ui/ui_behaviour.h"
#include "halley/ui/ui_event_handler.h"
#include "halley/input/input_keyboard.h"

using namespace Halley;

UIWidget::UIWidget(String id, Vector2f minSize, std::optional<UISizer> sizer, Vector4f innerBorder)
	: id(std::move(id))
	, size(minSize)
	, minSize(minSize)
	, innerBorder(innerBorder)
	, sizer(std::move(sizer))
{
	if (this->sizer) {
		this->sizer->reparent(*this);
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
	if (!isActive()) {
		return;
	}
	
	auto clip = painter.getClip();
	if (clip && !ignoreClip()) {
		if (!clip->overlaps(getRect())) {
			return;
		}
	}

	draw(painter);

	if (childLayerAdjustment == 0) {
		drawChildren(painter);
	} else {
		UIPainter p2 = painter.withAdjustedLayer(childLayerAdjustment);
		if (dontClipChildren) {
			p2 = p2.withNoClip();
		}
		drawChildren(p2);
	}

	drawAfterChildren(painter);
}

void UIWidget::doUpdate(UIWidgetUpdateType updateType, Time t, UIInputType inputType, JoystickType joystickType, Vector<std::shared_ptr<UIWidget>>& dst)
{
	if (updateType == UIWidgetUpdateType::Full || updateType == UIWidgetUpdateType::First) {
		setInputType(inputType);
		setJoystickType(joystickType);
	
		if (validator) {
			setEnabled(getValidator()->isEnabled());
		}

		checkActive();

		if (eventHandler) {
			eventHandler->pump();
		}
	}

	if (isActive()) {
		updateBehaviours(t);
		update(t, positionUpdated);
		positionUpdated = false;

		if (gamepadInputButtons && updateType == UIWidgetUpdateType::First) {
			onGamepadInput(gamepadInputResults, t);
		}

		addNewChildren(inputType);

		if (isActive()) {
			collectWidgetsForUpdating(dst);
		}
	}
}

void UIWidget::doPostUpdate()
{
	if (isActive()) {
		removeSizerDeadChildren();
		removeDeadChildren();
	}
}

void UIWidget::removeSizerDeadChildren()
{
	if (sizer) {
		for (auto& c : getChildren()) {
			if (!c->isAlive()) {
				sizer->remove(*c);
			}
		}
	}
}

void UIWidget::collectWidgetsForUpdating(Vector<std::shared_ptr<UIWidget>>& dst)
{
	for (auto& c: getChildren()) {
		assert(c->getRoot() == getRoot());
		dst.push_back(c);
	}
}

void UIWidget::collectWidgetsForRendering(size_t curRootIdx, Vector<std::pair<std::shared_ptr<UIWidget>, size_t>>& dst, Vector<std::shared_ptr<UIWidget>>& dstRoots)
{
	if (isActive()) {
		for (auto& c: getChildren()) {
			c->collectWidgetsForRendering(curRootIdx, dst, dstRoots);
		}
		if (hasRender()) {
			dst.emplace_back(shared_from_this(), curRootIdx);
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
			layoutSize = sizer->getLayoutMinimumSize(false);
			if (layoutSize.x > 0.1f || layoutSize.y > 0.1f) {
				layoutSize += Vector2f(border.x + border.z, border.y + border.w);
			}
		}
		return Vector2f::max(minSize, layoutSize);
	}
	return minSize;
}

void UIWidget::setRect(Rect4f rect, IUIElementListener* listener)
{
	setWidgetRect(rect);
	if (sizer) {
		const auto border = getInnerBorder();
		const auto p0 = getLayoutOriginPosition();
		const auto size = getLayoutSize(rect.getSize());
		if (listener) {
			onPreNotifySetRect(*listener);
		}
		sizer->setRect(Rect4f(p0 + Vector2f(border.x, border.y), p0 + size - Vector2f(border.z, border.w)), listener);
	} else {
		for (auto& c: getChildren()) {
			c->layout();
		}
	}
}

void UIWidget::onPreNotifySetRect(IUIElementListener& listener)
{
}

void UIWidget::layout(IUIElementListener* listener)
{
	checkActive();
	if (isActive()) {
		Vector2f minimumSize = getLayoutMinimumSize(false).ceil();
		Vector2f targetSize = Vector2f::max(shrinkOnLayout ? Vector2f() : size, minimumSize);
		setRect(Rect4f(getPosition(), getPosition() + targetSize), listener);

		alignAtAnchor();
		onLayout();
	}
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

const UIAnchor* UIWidget::getAnchor() const
{
	return anchor.get();
}

void UIWidget::setAnchor()
{
	anchor.reset();
}

std::optional<UISizer>& UIWidget::tryGetSizer()
{
	return sizer;
}

const std::optional<UISizer>& UIWidget::tryGetSizer() const
{
	return sizer;
}

UISizer& UIWidget::getSizer()
{
	if (!sizer) {
		throw Exception("UIWidget does not have a sizer.", HalleyExceptions::UI);
	}
	return sizer.value();
}

void UIWidget::setSizer(std::optional<UISizer> sizer)
{
	this->sizer = std::move(sizer);
	if (this->sizer) {
		this->sizer->reparent(*this);
	}
}

void UIWidget::add(std::shared_ptr<IUIElement> element, float proportion, Vector4f border, int fillFlags, size_t insertPos)
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
		sizer->add(element, proportion, border, fillFlags, insertPos);
	}
}

void UIWidget::addSpacer(float size)
{
	if (sizer) {
		sizer->addSpacer(size);
	}
}

void UIWidget::addStretchSpacer(float proportion)
{
	if (sizer) {
		sizer->addStretchSpacer(proportion);
	}
}

void UIWidget::remove(IUIElement& element)
{
	const auto widget = dynamic_cast<UIWidget*>(&element);
	if (widget) {
		removeChild(*widget);
		widget->destroy();
	} else {
		const auto sizer = dynamic_cast<UISizer*>(&element);
		if (sizer) {
			sizer->clear();
		}
		//throw Exception("Unimplemented: UIWidget::remove with sizer", HalleyExceptions::UI);
	}
	if (sizer) {
		sizer->remove(element);
	}
}

void UIWidget::clear()
{
	clearChildren();
	clearBehaviours();
}

void UIWidget::clearChildren()
{
	if (sizer) {
		sizer->clear();
	}
	UIParent::clear();
}

void UIWidget::setInteractWithMouse(bool enabled)
{
	mouseInteraction = enabled;
}

bool UIWidget::canInteractWithMouse() const
{
	return mouseInteraction;
}

bool UIWidget::canChildrenInteractWithMouse() const
{
	return true;
}

bool UIWidget::isFocused() const
{
	return focused;
}

void UIWidget::focus()
{
	if (!focused && canReceiveFocus()) {
		const auto root = getRoot();
		if (!root) {
			throw Exception("UIWidget must be added to root before calling focus()", HalleyExceptions::UI);
		}
		root->setFocus(shared_from_this());
	}
}

bool UIWidget::canPropagateMouseToChildren() const
{
	return propagateMouseToChildren;
}

void UIWidget::setPropagateMouseToChildren(bool enabled)
{
	propagateMouseToChildren = enabled;
}

void UIWidget::notifyWidgetUnderMouse(const std::shared_ptr<UIWidget>& widget)
{
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

Vector2f UIWidget::getPosition(bool ignoreOffset) const
{
	return ignoreOffset ? position : position + positionOffset;
}

void UIWidget::setPositionOffset(Vector2f offset)
{
	if (positionOffset != offset) {
		positionOffset = offset;
		positionUpdated = true;
	}
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

bool UIWidget::ignoreClip() const
{
	return sizer && sizer->getType() == UISizerType::Free && !getChildren().empty();
}

void UIWidget::setPosition(Vector2f pos)
{
	Expects(pos.isValid());
	
	position = pos;
	positionUpdated = true;
}

void UIWidget::setBorder(Vector4f border)
{
	if (auto* parentWidget = dynamic_cast<UIWidget*>(parent)) {
		if (auto& parentSizer = parentWidget->tryGetSizer()) {
			if (auto* entry = parentSizer->tryGetEntry(this)) {
				entry->setBorder(border);
				parentWidget->markAsNeedingLayout();
			}
		}
	}
}

void UIWidget::setMinSize(Vector2f size)
{
	Expects(size.isValid());
	
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

bool UIWidget::isMouseInside(Vector2f mousePos) const
{
	return getMouseRect().contains(mousePos);
}

void UIWidget::setMouseOver(bool mo)
{
	mouseOver = mo;
}

std::optional<std::shared_ptr<UIWidget>> UIWidget::prePressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	return {};
}

void UIWidget::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
}

void UIWidget::releaseMouse(Vector2f mousePos, int button)
{
}

void UIWidget::onMouseOver(Vector2f mousePos)
{
}

void UIWidget::onMouseOver(Vector2f mousePos, KeyMods keyMods)
{
	onMouseOver(mousePos);
}

void UIWidget::onMouseLeft(Vector2f mousePos)
{
}

bool UIWidget::isActive() const
{
	return activeByUser && activeByInput && alive;
}

bool UIWidget::isActiveInHierarchy() const
{
	return isActive() && (!getParent() || getParent()->isActiveInHierarchy());
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
		notifyActivationChange(isActive());
	}
}

void UIWidget::notifyActivationChange(bool active)
{
	onActiveChanged(active);
	for (auto& c: getChildren()) {
		c->notifyActivationChange(active);
	}
	for (auto& c: getChildrenWaiting()) {
		c->notifyActivationChange(active);
	}
}

void UIWidget::onActiveChanged(bool active)
{
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
	return root;
}

const UIRoot* UIWidget::getRoot() const
{
	return root;
}

void UIWidget::notifyDataBind(bool data, bool force) const
{
	if (dataBind) {
		if (force) {
			dataBind->setAcceptingDataFromWidget(true);
		}
		dataBind->onDataFromWidget(data);
	}
}

void UIWidget::notifyDataBind(int data, bool force) const
{
	if (dataBind) {
		if (force) {
			dataBind->setAcceptingDataFromWidget(true);
		}
		dataBind->onDataFromWidget(data);
	}
}

void UIWidget::notifyDataBind(float data, bool force) const
{
	if (dataBind) {
		if (force) {
			dataBind->setAcceptingDataFromWidget(true);
		}
		dataBind->onDataFromWidget(data);
	}
}

void UIWidget::notifyDataBind(const String& data, bool force) const
{
	if (dataBind) {
		if (force) {
			dataBind->setAcceptingDataFromWidget(true);
		}
		dataBind->onDataFromWidget(data);
	}
}

void UIWidget::notifyDataBind(const ConfigNode& data, bool force) const
{
	if (dataBind) {
		if (force) {
			dataBind->setAcceptingDataFromWidget(true);
		}
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
	forceAddChildren(lastInputType, true);
	layout();
}

void UIWidget::forceAddChildren(UIInputType inputType, bool forceRecursive)
{
	bool added = addNewChildren(inputType);
	if (added || forceRecursive) {
		for (auto& c: getChildren()) {
			c->forceAddChildren(inputType, forceRecursive);
		}
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

const Vector<std::shared_ptr<UIBehaviour>>& UIWidget::getBehaviours() const
{
	return behaviours;
}

UIInputType UIWidget::getLastInputType() const
{
	return lastInputType;
}

void UIWidget::onGamepadInput(const UIInputResults& input, Time time)
{
}

void UIWidget::setMouseClip(std::optional<Rect4f> clip, bool force)
{
	if (force || clip != mouseClip) {
		mouseClip = clip;
		for (auto& c: getChildren()) {
			c->setMouseClip(clip, force);
		}
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

	if (p) {
		if (!root) {
			auto* parentRoot = p->getRoot();
			if (parentRoot) {
				notifyTreeAddedToRoot(*parentRoot);
			}
		}
	} else {
		if (root) {
			notifyTreeRemovedFromRoot(*root);
		}
	}
	
	parent = p;

	if (parent) {
		parent->markAsNeedingLayout();
	}

	if (alive && anchor) {
		layout();
	}

	onParentChanged();
}

void UIWidget::notifyTreeAddedToRoot(UIRoot& root)
{
	if (this->root != &root) {
		this->lastInputType = root.getLastInputType();
		this->root = &root;
		onAddedToRoot(root);

		for (auto& c : getChildren()) {
			c->notifyTreeAddedToRoot(root);
		}
		for (auto& c : getChildrenWaiting()) {
			if (c) {
				c->notifyTreeAddedToRoot(root);
			}
		}
	}
}

void UIWidget::notifyTreeRemovedFromRoot(UIRoot& root)
{
	this->root = nullptr;
	onRemovedFromRoot(root);
	root.onWidgetRemoved(*this);

	for (auto& c: getChildren()) {
		c->notifyTreeRemovedFromRoot(root);
	}
	for (auto& c: getChildrenWaiting()) {
		if (c) {
			c->notifyTreeRemovedFromRoot(root);
		}
	}
}

UIParent* UIWidget::getParent() const
{
	return parent;
}

void UIWidget::destroy()
{
	if (alive && !destroying) {
		destroying = true;
		bool ok = onDestroyRequested();
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
	const bool wasActive = isActive();
	alive = false;
	updateActive(wasActive);
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

std::shared_ptr<UIWidget> UIWidget::tryGetAncestorWidget(const String& id)
{
	if (getId() == id) {
		return shared_from_this();
	} else if (auto* parent = getParent()) {
		return parent->tryGetAncestorWidget(id);
	} else {
		return {};
	}
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

void UIWidget::clearHandle(UIEventType type)
{
	getEventHandler().clearHandle(type);
}

void UIWidget::clearHandle(UIEventType type, String id)
{
	getEventHandler().clearHandle(type, id);
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

void UIWidget::setOnlyEnabledWithInputs(const Vector<UIInputType>& uiInput)
{
	onlyEnabledWithInputs = uiInput;
	setInputType(UIInputType::Keyboard);
}

const Vector<UIInputType>& UIWidget::getOnlyEnabledWithInput() const
{
	return onlyEnabledWithInputs;
}

void UIWidget::setInputButtons(const UIInputButtons& buttons)
{
	gamepadInputButtons = std::make_unique<UIInputButtons>(buttons);
}

Rect4f UIWidget::getMouseRect() const
{
	auto rect = getRect();
	if (mouseClip) {
		return rect.intersection(mouseClip.value());
	} else {
		return rect;
	}
}

void UIWidget::setToolTip(LocalisedString tip)
{
	if (!toolTip) {
		toolTip = std::make_unique<LocalisedString>(tip);
	} else {
		*toolTip = std::move(tip);
	}
}

bool UIWidget::hasStyle() const
{
	return !styles.empty();
}

const Vector<UIStyle>& UIWidget::getStyles() const
{
	return styles;
}

void UIWidget::setResultValue(ConfigNode data)
{
}

ConfigNode UIWidget::getResultValue()
{
	return ConfigNode();
}

void UIWidget::fitToRoot()
{
	if (root) {
		auto rect = root->getRect();
		if (getRect() != rect) {
			setPosition(rect.getP1());
			setMinSize(rect.getSize());
			layout();
		}
	}
}

std::optional<Vector2f> UIWidget::transformToChildSpace(Vector2f pos) const
{
	return pos;
}

std::optional<MouseCursorMode> UIWidget::getMouseCursorMode() const
{
	return std::nullopt;
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

bool UIWidget::hasRender() const
{
	return false;
}

void UIWidget::onPreRender()
{
}

void UIWidget::render(RenderContext& render) const
{
}

RenderContext UIWidget::getRenderContextForChildren(RenderContext& rc)
{
	return rc;
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

void UIWidget::onFocus(bool byClicking)
{
}

void UIWidget::onFocusLost()
{
}

TextInputData* UIWidget::getTextInputData()
{
	return nullptr;
}

void UIWidget::onLayout()
{
}

bool UIWidget::onDestroyRequested()
{
	return true;
}

void UIWidget::sendEvent(UIEvent event, bool includeSelf) const
{
	if (includeSelf && eventHandler && eventHandler->canHandle(event)) {
		eventHandler->queue(event, UIEventDirection::Up);
	} else if (parent && canSendEvents) {
		parent->sendEvent(std::move(event), true);
	}
}

void UIWidget::sendEventDown(const UIEvent& event, bool includeSelf) const
{
	if (includeSelf && eventHandler && eventHandler->canHandle(event)) {
		eventHandler->queue(event, UIEventDirection::Down);
	} else {
		for (const auto& c: getChildren()) {
			c->sendEventDown(event, true);
		}
		for (const auto& c: getChildrenWaiting()) {
			c->sendEventDown(event, true);
		}
	}
}

std::optional<AudioHandle> UIWidget::playSound(const String& eventName)
{
	if (!eventName.isEmpty()) {
		if (auto root = getRoot()) {
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

bool UIWidget::canReceiveFocus() const
{
	return false;
}

bool UIWidget::canReceiveMouseExclusive() const
{
	return true;
}

std::shared_ptr<UIWidget> UIWidget::getFocusableOrAncestor()
{
	if (canReceiveFocus()) {
		return shared_from_this();
	}
	if (parent) {
		auto widget = dynamic_cast<UIWidget*>(parent);
		if (widget) {
			return widget->getFocusableOrAncestor();
		}
	}
	return {};
}

void UIWidget::onAddedToRoot(UIRoot& root)
{
}

void UIWidget::onRemovedFromRoot(UIRoot& root)
{
}

void UIWidget::onChildAdded(UIWidget& child)
{
	auto* root = getRoot();
	if (root) {
		child.notifyTreeAddedToRoot(*root);
	}
	child.setMouseClip(mouseClip, true);
}

void UIWidget::onMakeUI()
{
}

LocalisedString UIWidget::getToolTip() const
{
	return toolTip ? *toolTip : LocalisedString();
}

void UIWidget::checkActive()
{
}

void UIWidget::playStyleSound(const String& keyId)
{
	if (!styles.empty()) {
		if (styles.at(0).hasString(keyId)) {
			const auto& eventId = styles.at(0).getString(keyId);
			if (!eventId.isEmpty()) {
				playSound(eventId);
			}
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

void UIWidget::resetInputResults()
{
	gamepadInputResults.reset();
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

void UIWidget::bindData(const String& childId, ConfigNode initialValue, UIDataBindConfigNode::WriteCallback callback)
{
	auto widget = tryGetWidget(childId);
	if (widget) {
		widget->setDataBind(std::make_shared<UIDataBindConfigNode>(std::move(initialValue), std::move(callback)));
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

Vector2f UIWidget::getLayoutSize(Vector2f size) const
{
	return size;
}

void UIWidget::updateInputDevice(const InputDevice& inputDevice)
{
}

bool UIWidget::onKeyPress(KeyboardKeyPress key)
{
	return false;
}

void UIWidget::receiveKeyPress(KeyboardKeyPress key)
{
	const bool handled = onKeyPress(key);
	if (!handled) {
		getParent()->receiveKeyPress(key);
	}
}

UIGamepadInput::Priority UIWidget::getInputPriority() const
{
	return focused && gamepadInputButtons->boostPriorityWhenFocused ? UIGamepadInput::Priority::Focused : gamepadInputButtons->priorityLevel;
}

void UIWidget::setChildLayerAdjustment(int delta)
{
	childLayerAdjustment = delta;
}

int UIWidget::getChildLayerAdjustment() const
{
	return childLayerAdjustment;
}

void UIWidget::setNoClipChildren(bool noClip)
{
	dontClipChildren = noClip;
}

bool UIWidget::getNoClipChildren() const
{
	return dontClipChildren;
}
