#include "ui_parent.h"
#include "ui_widget.h"
#include "halley/support/logger.h"
using namespace Halley;

void UIParent::addChild(std::shared_ptr<UIWidget> widget)
{
	Expects(widget->getParent() == nullptr || widget->getParent() == this);

	if (widget->getParent() == nullptr) {
		widget->setParent(this);
		childrenWaiting.push_back(widget);
	}
}

void UIParent::removeChild(UIWidget& widget)
{
	Expects(widget.getParent() == this);
	widget.setParent(nullptr);

	children.erase(std::remove_if(children.begin(), children.end(), [&] (auto& c)
	{
		return c.get() == &widget;
	}), children.end());
}

bool UIParent::addNewChildren(UIInputType inputType)
{
	bool addedAny = !childrenWaiting.empty();

	for (auto& c: childrenWaiting) {
		c->setInputType(inputType);
		children.emplace_back(std::move(c));
	}
	childrenWaiting.clear();

	return addedAny;
}

bool UIParent::removeDeadChildren()
{
	auto before = children.size();

	children.erase(std::remove_if(children.begin(), children.end(), [&] (auto& c)
	{
		return !c->isAlive();
	}), children.end());

	return before != children.size();
}

bool UIParent::isWaitingToSpawnChildren() const
{
	return !childrenWaiting.empty();
}

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
	auto lists = { children, childrenWaiting };
	for (auto& cs : lists) {
		for (auto& c: cs) {
			if (c->isAlive()) {
				if (c->getId() == id) {
					return c;
				}
				auto c2 = c->getWidget(id);
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
