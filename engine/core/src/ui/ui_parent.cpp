#include "ui/ui_parent.h"
#include "ui/ui_widget.h"
using namespace Halley;

void UIParent::addChild(std::shared_ptr<UIWidget> widget)
{
	widget->setParent(*this);
	childrenWaiting.push_back(widget);
	topChildChanged = true;
}

void UIParent::removeChild(UIWidget& widget)
{
	children.erase(std::remove_if(children.begin(), children.end(), [&] (auto& c)
	{
		return c.get() == &widget;
	}), children.end());
	topChildChanged = true;
}

void UIParent::addNewChildren()
{
	for (auto& c: childrenWaiting) {
		children.emplace_back(std::move(c));
	}
	childrenWaiting.clear();
	for (auto& c: children) {
		c->addNewChildren();
	}
}

void UIParent::removeDeadChildren()
{
	children.erase(std::remove_if(children.begin(), children.end(), [&] (auto& c)
	{
		return !c->isAlive();
	}), children.end());
	for (auto& c: children) {
		c->removeDeadChildren();
	}
}

std::vector<std::shared_ptr<UIWidget>>& UIParent::getChildren()
{
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
			if (c->getId() == id) {
				return c;
			}
			auto c2 = c->getWidget(id);
			if (c2) {
				return c2;
			}
		}
	}
	return {};
}
