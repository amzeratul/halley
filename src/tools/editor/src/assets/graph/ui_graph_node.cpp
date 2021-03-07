#include "ui_graph_node.h"

using namespace Halley;

UIGraphNode::UIGraphNode(String id, UIFactory& factory)
	: UIWidget(std::move(id), {}, UISizer())
{
	factory.loadUI(*this, "ui/halley/graph_node");
}

bool UIGraphNode::canInteractWithMouse() const
{
	return true;
}

void UIGraphNode::pressMouse(Vector2f mousePos, int button)
{}

void UIGraphNode::releaseMouse(Vector2f mousePos, int button)
{}

void UIGraphNode::onMouseOver(Vector2f mousePos)
{}
