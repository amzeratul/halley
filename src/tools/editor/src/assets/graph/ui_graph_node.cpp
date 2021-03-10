#include "ui_graph_node.h"

using namespace Halley;

UIGraphNode::UIGraphNode(const RenderGraphDefinition::Node& node, UIFactory& factory)
	: UIWidget(node.id, {}, UISizer())
	, node(node)
{
	factory.loadUI(*this, "ui/halley/graph_node");
}

void UIGraphNode::onMakeUI()
{
	getWidgetAs<UILabel>("title")->setText(LocalisedString::fromUserString(getId()));
}

bool UIGraphNode::canInteractWithMouse() const
{
	return true;
}

void UIGraphNode::pressMouse(Vector2f mousePos, int button)
{
	if (button == 0) {
		Vector2f parentPos;
		auto* parent = dynamic_cast<UIWidget*>(getParent());
		if (parent) {
			auto& sizer = parent->getSizer();
			const size_t n = sizer.size();
			for (size_t i = 0; i < n; ++i) {
				if (sizer[i].getPointer().get() == this) {
					parentPos = sizer[i].getPosition();
				}
			}
		}
		
		drag = parentPos - mousePos;
	}
}

void UIGraphNode::releaseMouse(Vector2f mousePos, int button)
{
	if (button == 0) {
		drag.reset();
	}
}

void UIGraphNode::onMouseOver(Vector2f mousePos)
{
	if (drag) {
		const auto newPos = mousePos + drag.value();
		
		auto* parent = dynamic_cast<UIWidget*>(getParent());
		if (parent) {
			auto& sizer = parent->getSizer();
			const size_t n = sizer.size();
			for (size_t i = 0; i < n; ++i) {
				if (sizer[i].getPointer().get() == this) {
					sizer[i].setPosition(newPos);
				}
			}
			sizer.sortItems([] (const UISizerEntry& a, const UISizerEntry& b) -> bool { return a.getPosition().y < b.getPosition().y; });
			parent->layout();
		}
	}
}

bool UIGraphNode::isFocusLocked() const
{
	return !!drag;
}
