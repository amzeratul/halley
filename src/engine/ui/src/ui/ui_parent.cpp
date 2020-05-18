#include "ui_parent.h"
#include "ui_widget.h"
#include "halley/support/logger.h"
using namespace Halley;

UIParent::~UIParent()
{
	for (auto& c: children) {
		c->setParent(nullptr);
	}
}

std::optional<float> UIParent::getMaxChildWidth() const
{
	return std::optional<float>();
}

void UIParent::addChild(std::shared_ptr<UIWidget> widget)
{
	Expects(widget->getParent() == nullptr || widget->getParent() == this);

	if (widget->getParent() == nullptr) {
		widget->setParent(this);
		childrenWaiting.push_back(widget);
	}

	markAsNeedingLayout();
}

void UIParent::removeChild(UIWidget& widget)
{
	Expects(widget.getParent() == this);
	widget.setParent(nullptr);

	children.erase(std::remove_if(children.begin(), children.end(), [&] (auto& c)
	{
		return c.get() == &widget;
	}), children.end());

	markAsNeedingLayout();
}

void UIParent::clear()
{
	if (children.empty()) {
		return;
	}

	for (auto& c: children) {
		c->setParent(nullptr);
	}
	children.clear();
	markAsNeedingLayout();
}

bool UIParent::addNewChildren(UIInputType inputType)
{
	if (childrenWaiting.empty()) {
		return false;
	}

	for (auto& c: childrenWaiting) {
		c->setInputType(inputType);
		onChildAdded(*children.emplace_back(std::move(c)));
	}
	childrenWaiting.clear();

	markAsNeedingLayout();
	onChildrenAdded();
	return true;
}

bool UIParent::removeDeadChildren()
{
	auto before = children.size();

	children.erase(std::remove_if(children.begin(), children.end(), [&] (auto& c)
	{
		return !c->isAlive();
	}), children.end());

	const bool removedAny = before != children.size();
	if (removedAny) {
		markAsNeedingLayout();
		onChildrenRemoved();
	}
	return removedAny;
}

bool UIParent::isWaitingToSpawnChildren() const
{
	return !childrenWaiting.empty();
}

void UIParent::markAsNeedingLayout() {}

std::vector<std::shared_ptr<UIWidget>>& UIParent::getChildren()
{
	/*
	if (!childrenWaiting.empty()) {
		Logger::logWarning("Attempting to retrieve children of component which has unspawned children");
	}
	*/

	return children;
}

const std::vector<std::shared_ptr<UIWidget>>& UIParent::getChildren() const
{
	return children;
}

std::shared_ptr<UIWidget> UIParent::getWidget(const String& id)
{
	if (getId() == id) {
		auto widgetThis = dynamic_cast<UIWidget*>(this);
		if (!widgetThis) {
			throw Exception("Found \"" + id + "\", but it is not a widget", HalleyExceptions::UI);
		}
		return widgetThis->shared_from_this();
	}

	auto widget = doGetWidget(id);
	if (!widget) {
		throw Exception("Widget with id \"" + id + "\" not found.", HalleyExceptions::UI);
	}
	return widget;
}

std::shared_ptr<UIWidget> UIParent::tryGetWidget(const String& id)
{
	if (getId() == id) {
		auto widgetThis = dynamic_cast<UIWidget*>(this);
		if (widgetThis) {
			return widgetThis->shared_from_this();
		} else {
			return {};
		}		
	}

	return doGetWidget(id);
}

std::shared_ptr<UIWidget> UIParent::doGetWidget(const String& id) const
{
	auto lists = { children, childrenWaiting };
	for (auto& cs : lists) {
		for (auto& c: cs) {
			if (c->isAlive()) {
				if (c->getId() == id) {
					return c;
				}
				auto c2 = c->doGetWidget(id);
				if (c2) {
					return c2;
				}
			}
		}
	}
	return {};
}


bool UIParent::isDescendentOf(const UIWidget& ancestor) const
{
	return false;
}

bool UIParent::isActiveInHierarchy() const
{
	return true;
}
