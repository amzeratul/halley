#include "ui/ui_root.h"
#include "ui/ui_widget.h"

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

UIRoot& UIRoot::getRoot()
{
	return *this;
}

void UIRoot::addWidget(UIWidget& widget)
{
	widgets.push_back(&widget);
}

void UIRoot::removeWidget(UIWidget& widget)
{
	widgets.erase(std::remove(widgets.begin(), widgets.end(), &widget), widgets.end());
}

void UIRoot::update(Time t, Vector2f mousePos, bool leftClick, bool rightClick)
{
	for (auto& c: getChildren()) {
		c->layout();
	}
}
