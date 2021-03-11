#include "ui_graph_node.h"

#include "graph_editor.h"

using namespace Halley;

UIGraphNode::UIGraphNode(GraphEditor& editor, const RenderGraphDefinition::Node& node, UIFactory& factory)
	: UIWidget(node.id, {}, UISizer())
	, editor(editor)
	, factory(factory)
	, node(node)
{
	factory.loadUI(*this, "ui/halley/graph_node");
}

void UIGraphNode::onMakeUI()
{
	getWidgetAs<UILabel>("title")->setText(LocalisedString::fromUserString(getId()));

	{
		const auto& inputPins = getWidget("inputPins");
		int i = 0;
		for (const auto& inputPin: node.getInputPins()) {
			auto pin = std::make_shared<UIImage>("inputPin" + toString(i++), Sprite().setImage(factory.getResources(), "halley_ui/ui_render_graph_node_pin.png"));
			inputPinWidgets.push_back(pin);
			inputPins->add(pin);
		}
	}

	{
		const auto& outputPins = getWidget("outputPins");
		int i = 0;
		for (const auto& outputPin: node.getOutputPins()) {
			auto pin = std::make_shared<UIImage>("outputPin" + toString(i++), Sprite().setImage(factory.getResources(), "halley_ui/ui_render_graph_node_pin.png"));
			outputPinWidgets.push_back(pin);
			outputPins->add(pin);
		}
	}
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

std::shared_ptr<UIWidget> UIGraphNode::getPinWidget(bool outputPin, uint8_t pinId)
{
	return (outputPin ? outputPinWidgets : inputPinWidgets).at(pinId);
}

