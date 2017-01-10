#include "ui/ui_root.h"
#include "ui/ui_widget.h"
#include "graphics/sprite/sprite_painter.h"

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

const std::vector<std::shared_ptr<UIWidget>>& UIParent::getChildren() const
{
	return children;
}

UIPainter::UIPainter(SpritePainter& painter, int mask, int layer)
	: painter(painter)
	, mask(mask)
	, layer(layer)
	, n(0)
{
}

void UIPainter::draw(const Sprite& sprite)
{
	painter.add(sprite, mask, layer, float(n++));
}

void UIPainter::draw(const TextRenderer& sprite)
{
	painter.add(sprite, mask, layer, float(n++));
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

	for (auto& c: widgets) {
		c->update(t);
	}
}

void UIRoot::draw(SpritePainter& painter, int mask, int layer)
{
	UIPainter p(painter, mask, layer);

	for (auto& c: getChildren()) {
		c->draw(p);
	}
}

