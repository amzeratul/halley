#include "ui_graph_node.h"

#include "graph_editor.h"

using namespace Halley;

UIGraphNode::UIGraphNode(String id, GraphEditor& editor, UIFactory& factory, UIStyle style)
	: UIWidget(std::move(id), {}, UISizer())
	, editor(editor)
	, factory(factory)
	, style(std::move(style))
{
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

UIRenderGraphNode::UIRenderGraphNode(GraphEditor& editor, const RenderGraphDefinition::Node& node, UIFactory& factory, UIStyle style)
	: UIGraphNode(node.id, editor, factory, style)
	, node(node)
{
	setPosition(node.position);
	factory.loadUI(*this, "ui/halley/graph_node");
}

void UIRenderGraphNode::onMakeUI()
{
	getWidgetAs<UILabel>("title")->setText(LocalisedString::fromUserString(getId()));

	const auto bg = style.getSprite(node.method + "Background");
	getWidgetAs<UIImage>("background")->getSprite().setColour(bg.getColour());

	{
		const auto& inputPins = getWidget("inputPins");
		int i = 0;
		for (const auto& inputPin: node.getInputPins()) {
			auto pin = std::make_shared<UIImage>("inputPin" + toString(i++), Sprite().setImage(factory.getResources(), "halley_ui/ui_render_graph_node_pin.png"));
			pin->getSprite().setColour(editor.getColourForPinType(inputPin));
			inputPinWidgets.push_back(pin);
			inputPins->add(pin);
		}
	}
	
	{
		const auto& outputPins = getWidget("outputPins");
		int i = 0;
		for (const auto& outputPin: node.getOutputPins()) {
			auto pin = std::make_shared<UIImage>("outputPin" + toString(i++), Sprite().setImage(factory.getResources(), "halley_ui/ui_render_graph_node_pin.png"));
			pin->getSprite().setColour(editor.getColourForPinType(outputPin));
			outputPinWidgets.push_back(pin);
			outputPins->add(pin);
		}
	}

	const auto fields = getWidget("fields");
	auto addField = [&] (std::string_view imageName, std::string_view data)
	{
		fields->add(std::make_shared<UIImage>(Sprite().setImage(factory.getResources(), imageName)), 0, Vector4f(0, -2, 0, -2));
		fields->add(std::make_shared<UILabel>("", factory.getStyle("labelSmall"), LocalisedString::fromUserString(data)), 1);
	};

	if (node.method == RenderGraphMethod::Paint) {
		addField("entity_icons/tag.png", node.methodParameters["paintId"].asString(""));
		addField("entity_icons/camera.png", node.methodParameters["cameraId"].asString(""));
	} else if (node.method == RenderGraphMethod::Overlay) {
		addField("ui/assetTypes/materialDefinition.png", node.methodParameters["material"].asString(""));
	}
}

