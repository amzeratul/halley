#include "halley/ui/ui_parent.h"
#include "halley/ui/ui_widget.h"
#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"
using namespace Halley;

UIParent::~UIParent()
{
	for (auto& c: children) {
		c->setParent(nullptr);
	}
}

std::optional<float> UIParent::getMaxChildWidth() const
{
	return {};
}

UIWidget& UIParent::addChild(std::shared_ptr<UIWidget> widget)
{
	HalleyAssertDev(widget);
	HalleyAssertDev(widget->getParent() == nullptr || widget->getParent() == this);

	if (widget->getParent() == nullptr) {
		widget->setParent(this);
		childrenWaiting.push_back(std::move(widget));
	}

	markAsNeedingLayout();

	return *childrenWaiting.back();
}

void UIParent::removeChild(UIWidget& widget)
{
	HalleyAssertDev(widget.getParent() == this);
	widget.setParent(nullptr);

	std_ex::erase_if(children, [&] (auto& c) { return c.get() == &widget; });

	markAsNeedingLayout();
}

void UIParent::clear()
{
	if (children.empty() && childrenWaiting.empty()) {
		return;
	}

	for (auto& c: children) {
		c->setParent(nullptr);
	}
	for (auto& c: childrenWaiting) {
		c->setParent(nullptr);
	}
	children.clear();
	childrenWaiting.clear();
	markAsNeedingLayout();
}

bool UIParent::addNewChildren(UIInputType inputType)
{
	if (childrenWaiting.empty()) {
		return false;
	}

	for (auto& c: childrenWaiting) {
		HalleyAssertDev(c->getRoot() == getRoot());
		c->setInputType(inputType);
		onChildAdded(*children.emplace_back(std::move(c)));
	}
	childrenWaiting.clear();

	std::sort(children.begin(), children.end(), [=] (const std::shared_ptr<UIWidget>& a, const std::shared_ptr<UIWidget>& b) {
		return a->getRootPriority() < b->getRootPriority();
	});

	markAsNeedingLayout();
	onChildrenAdded();
	return true;
}

bool UIParent::removeDeadChildren()
{
	for (auto& child: children) {
		if (!child->isAlive() && child->getParent()) {
			child->setParent(nullptr);
		}
	}

	const bool removedAny = std_ex::erase_if(children, [&] (auto& c) { return !c->isAlive(); });

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

Vector<std::shared_ptr<UIWidget>>& UIParent::getChildren()
{
	/*
	if (!childrenWaiting.empty()) {
		Logger::logWarning("Attempting to retrieve children of component which has unspawned children");
	}
	*/

	return children;
}

const Vector<std::shared_ptr<UIWidget>>& UIParent::getChildren() const
{
	return children;
}

Vector<std::shared_ptr<UIWidget>>& UIParent::getChildrenWaiting()
{
	return childrenWaiting;
}

const Vector<std::shared_ptr<UIWidget>>& UIParent::getChildrenWaiting() const
{
	return childrenWaiting;
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

std::shared_ptr<UIWidget> UIParent::tryGetAncestorWidget(const String& id)
{
	return {};
}

int UIParent::getAbsoluteChildLayerAdjustment() const
{
	return 0;
}

std::shared_ptr<UIWidget> UIParent::doGetWidget(const String& id) const
{
	const auto lists = { gsl::span<const std::shared_ptr<UIWidget>>(children), gsl::span<const std::shared_ptr<UIWidget>>(childrenWaiting) };
	for (const auto& cs : lists) {
		for (const auto& c: cs) {
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
