#include "ui/ui_root.h"
#include "ui/ui_widget.h"

using namespace Halley;

void UIParent::addChild(UIWidget& widget)
{
	children.push_back(&widget);
}

void UIParent::removeChild(UIWidget& widget)
{
	children.erase(std::remove(children.begin(), children.end(), &widget), children.end());
}

std::vector<UIWidget*>& UIParent::getChildren()
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
