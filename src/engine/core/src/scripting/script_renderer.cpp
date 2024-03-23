#include "halley/scripting/script_renderer.h"
#include "halley/entity/world.h"
#include "halley/graphics/painter.h"
#include "halley/utils/algorithm.h"
#include "halley/scripting/script_graph.h"
#include "halley/scripting/script_node_type.h"
using namespace Halley;

#ifndef DONT_INCLUDE_HALLEY_HPP
#define DONT_INCLUDE_HALLEY_HPP
#endif
#include "halley/entity/components/transform_2d_component.h"


ScriptRenderer::ScriptRenderer(Resources& resources, const World* world, const ScriptNodeTypeCollection& nodeTypeCollection, float nativeZoom)
	: BaseGraphRenderer(resources, nativeZoom)
	, world(world)
	, nodeTypeCollection(nodeTypeCollection)
{
	nodeBg = Sprite().setImage(resources, "halley_ui/ui_float_solid_window.png").setPivot(Vector2f(0.5f, 0.5f));
	nodeBgOutline = Sprite().setImage(resources, "halley_ui/ui_float_solid_window_outline.png").setPivot(Vector2f(0.5f, 0.5f));
	destructorBg = Sprite().setImage(resources, "halley_ui/script_destructor_bg.png").setPivot(Vector2f(0.5f, 0.0f));
	destructorIcon = Sprite().setImage(resources, "halley_ui/script_destructor_icon.png").setPivot(Vector2f(0.5f, 0.5f));
	pinSprite = Sprite().setImage(resources, "halley_ui/ui_render_graph_node_pin.png").setPivot(Vector2f(0.5f, 0.5f));
	labelText
		.setFont(resources.get<Font>("Ubuntu Bold"))
		.setSize(14)
		.setColour(Colour(1, 1, 1))
		.setOutlineColour(Colour(0, 0, 0))
		.setOutline(1);
}

void ScriptRenderer::setState(const ScriptState* scriptState)
{
	state = scriptState;
}

ScriptRenderer::NodeDrawMode ScriptRenderer::getNodeDrawMode(GraphNodeId nodeId) const
{
	NodeDrawMode drawMode;
	if (state) {
		// Rendering in-game, with execution state
		const auto nodeIntrospection = state->getNodeIntrospection(nodeId);
		drawMode.activationTime = nodeIntrospection.activationTime < 0.5f ? nodeIntrospection.activationTime * 2 : std::numeric_limits<float>::infinity();
		if (nodeIntrospection.state == ScriptState::NodeIntrospectionState::Active) {
			drawMode.type = NodeDrawModeType::Active;
			drawMode.time = nodeIntrospection.time;
		} else if (nodeIntrospection.state == ScriptState::NodeIntrospectionState::Unvisited) {
			drawMode.type = NodeDrawModeType::Unvisited;
		}
	} else {
		// Rendering in editor
		const bool highlightThis = highlightNode && highlightNode->nodeId == nodeId;
		if (highlightThis && highlightNode->element.type == GraphElementType(ScriptNodeElementType::Node)) {
			drawMode.type = NodeDrawModeType::Highlight;
		}
	}
	drawMode.selected = std_ex::contains(selectedNodes, nodeId);
	return drawMode;
}

GraphPinSide ScriptRenderer::getSide(GraphNodePinType pinType) const
{
	switch (ScriptNodeElementType(pinType.type)) {
	case ScriptNodeElementType::TargetPin:
		if (!pinType.forceHorizontal) {
			return pinType.direction == GraphNodePinDirection::Input ? GraphPinSide::Top : GraphPinSide::Bottom;
		} else {
			[[fallthrough]];
		}
	case ScriptNodeElementType::ReadDataPin:
	case ScriptNodeElementType::WriteDataPin:
	case ScriptNodeElementType::FlowPin:
		return pinType.direction == GraphNodePinDirection::Input ? GraphPinSide::Left : GraphPinSide::Right;
	default:
		return GraphPinSide::Undefined;
	}
}

String ScriptRenderer::getDebugDisplayValue(uint16_t id) const
{
	const auto iter = debugDisplayValues.find(id);
	if (iter != debugDisplayValues.end()) {
		return iter->second;
	}
	return "";
}

void ScriptRenderer::drawNodeBackground(Painter& painter, Vector2f basePos, const BaseGraphNode& node, float curZoom, float posScale, NodeDrawMode drawMode)
{
	const auto* nodeType = dynamic_cast<const IScriptNodeType*>(tryGetNodeType(node.getType()));
	if (!nodeType) {
		return;
	}

	// Destructor
	if (nodeType->hasDestructor(dynamic_cast<const ScriptGraphNode&>(node)) && nodeType->showDestructor()) {
		const auto pos = ((basePos + node.getPosition() * posScale) * curZoom).round() / curZoom;
		const auto [col, iconCol, borderAlpha] = getNodeColour(*nodeType, drawMode);

		destructorBg.clone()
			.setColour(col.multiplyLuma(0.6f))
			.setPosition(pos + Vector2f(0, 29) / curZoom)
			.setScale(1.0f / curZoom)
			.draw(painter);

		destructorIcon.clone()
			.setPosition(pos + Vector2f(0, 36) / curZoom)
			.setScale(1.0f / curZoom)
			.draw(painter);
	}
}

void ScriptRenderer::drawNode(Painter& painter, Vector2f basePos, const BaseGraphNode& node, float curZoom, float posScale, NodeDrawMode drawMode, std::optional<GraphNodePinType> highlightElement, GraphPinId highlightElementId)
{
	const auto* nodeType = dynamic_cast<const IScriptNodeType*>(tryGetNodeType(node.getType())); // TODO: remove cast
	if (!nodeType) {
		return;
	}

	const Vector2f border = Vector2f(18, 18);
	const Vector2f nodeSize = getNodeSize(*nodeType, node, curZoom);
	const auto pos = ((basePos + node.getPosition() * posScale) * curZoom).round() / curZoom;
	const auto [c, iconCol, borderAlpha] = getNodeColour(*nodeType, drawMode);
	auto col = c; // Clang doesn't seem to like lambda capturing (drawLabel, below) from a structured binding

	// Node body
	nodeBg.clone()
		.setColour(col)
		.setPosition(pos)
		.scaleTo(nodeSize + border)
		.setSize(nodeBg.getSize() / curZoom)
		.setSliceScale(1.0f / curZoom)
		.draw(painter);

	if (borderAlpha > 0.0001f) {
		nodeBgOutline.clone()
			.setPosition(pos)
			.scaleTo(nodeSize + border)
			.setSize(nodeBg.getSize() / curZoom)
			.setSliceScale(1.0f / curZoom)
			.setColour(Colour4f(1, 1, 1, borderAlpha))
			.draw(painter);
	}

	const auto label = nodeType->getLabel(node);
	const auto classification = nodeType->getClassification();
	const auto largeLabel = classification == ScriptNodeClassification::DebugDisplay ? getDebugDisplayValue(node.getId()) : nodeType->getLargeLabel(node);
	const Vector2f iconOffset = label.isEmpty() ? Vector2f() : Vector2f(0, -8.0f / curZoom).round();

	auto drawLabel = [&, col](const String& text, Vector2f pos, float size, float maxWidth, bool split)
	{
		auto labelCopy = labelText.clone()
			.setPosition(pos)
			.setSize(size)
			.setOutline(8.0f / curZoom)
			.setOutlineColour(col.multiplyLuma(0.75f))
			.setOffset(Vector2f(0.5f, 0.5f))
			.setAlignment(0.0f);

		if (split) {
			labelCopy.setText(labelCopy.split(text, maxWidth));
		} else {
			labelCopy.setText(text);
			const auto extents = labelCopy.getExtents();
			if (extents.x > maxWidth) {
				labelCopy.setSize(size * maxWidth / extents.x);
			}
		}

		labelCopy
			.draw(painter);
	};
	
	// Icon
	const auto& icon = getIconByName(nodeType->getIconName(node));
	if (icon.hasMaterial() && classification != ScriptNodeClassification::Comment) {
		icon.clone()
			.setPosition(pos + iconOffset)
			.setScale(1.0f / curZoom)
			.setColour(iconCol.multiplyAlpha(classification == ScriptNodeClassification::DebugDisplay ? 0.05f : (largeLabel.isEmpty() ? 1.0f : 0.25f)))
			.draw(painter);
	}

	// Large label
	if (!largeLabel.isEmpty()) {
		const bool isComment = classification == ScriptNodeClassification::Comment;
		const auto fontSize = isComment ? 14.0f : 18.0f;
		const auto margin = isComment ? 20.0f : 10.0f;
		drawLabel(largeLabel, pos + iconOffset, fontSize / curZoom, (nodeSize.x - margin) / curZoom, isComment);
	}

	// Label
	if (!label.isEmpty()) {
		drawLabel(label, pos + Vector2f(0, 18.0f / curZoom).round(), 14 / curZoom, (nodeSize.x - 10.0f) / curZoom, false);
	}

	// Draw pins
	const auto& pins = nodeType->getPinConfiguration(node);
	for (size_t i = 0; i < pins.size(); ++i) {
		const auto& pinType = pins[i];
		const auto circle = getNodeElementArea(*nodeType, basePos, node, i, curZoom, posScale);
		const auto baseCol = getPinColour(pinType);
		const auto col = highlightElement == pinType && highlightElementId == i ? baseCol.inverseMultiplyLuma(0.3f) : baseCol;
		pinSprite.clone()
			.setPosition(circle.getCentre())
			.setColour(col)
			.setScale(1.0f / curZoom)
			.draw(painter);
	}
}

Vector2f ScriptRenderer::getNodeSize(const IGraphNodeType& nodeType, const BaseGraphNode& node, float curZoom) const
{
	if (auto size = nodeType.getNodeSize(node, curZoom)) {
		return size.value();
	}

	switch (dynamic_cast<const IScriptNodeType&>(nodeType).getClassification()) {
	case ScriptNodeClassification::Variable:
		return Vector2f(100, 40);
	case ScriptNodeClassification::Comment:
		return getCommentNodeSize(node, curZoom);
	case ScriptNodeClassification::DebugDisplay:
		return Vector2f(150, 60);
	default:
		return Vector2f(60, 60);
	}
}

Vector2f ScriptRenderer::getCommentNodeSize(const BaseGraphNode& node, float curZoom) const
{
	const auto text = labelText.clone()
		.setSize(14);

	const float maxWidth = 250.0f;
	const auto split = text.split(node.getSettings()["comment"].asString(""), maxWidth);
	const auto extents = text.getExtents(split);

	return Vector2f::max(Vector2f(40, 40), extents + Vector2f(20, 20));
}

void ScriptRenderer::setDebugDisplayData(HashMap<int, String> values)
{
	debugDisplayValues = std::move(values);
}

Colour4f ScriptRenderer::getScriptNodeColour(const IScriptNodeType& nodeType)
{
	switch (nodeType.getClassification()) {
	case ScriptNodeClassification::Terminator:
		return Colour4f(0.97f, 0.35f, 0.35f);
	case ScriptNodeClassification::Action:
		return Colour4f(0.07f, 0.84f, 0.09f);
	case ScriptNodeClassification::Variable:
		return Colour4f(0.91f, 0.71f, 0.0f);
	case ScriptNodeClassification::Expression:
		return Colour4f(1.0f, 0.64f, 0.14f);
	case ScriptNodeClassification::FlowControl:
		return Colour4f(0.35f, 0.55f, 0.97f);
	case ScriptNodeClassification::State:
		return Colour4f(0.75f, 0.35f, 0.97f);
	case ScriptNodeClassification::Function:
		return Colour4f(1.00f, 0.49f, 0.68f);
	case ScriptNodeClassification::NetworkFlow:
		return Colour4f(0.15f, 0.85f, 0.98f);
	case ScriptNodeClassification::Comment:
		return Colour4f(0.25f, 0.25f, 0.3f);
	case ScriptNodeClassification::DebugDisplay:
		return Colour4f(0.1f, 0.1f, 0.15f);
	case ScriptNodeClassification::Unknown:
		return Colour4f(0.2f, 0.2f, 0.2f);
	}
	return Colour4f(0.2f, 0.2f, 0.2f);
}

Colour4f ScriptRenderer::getBaseNodeColour(const IGraphNodeType& nodeType) const
{
	return getScriptNodeColour(dynamic_cast<const IScriptNodeType&>(nodeType));
}

bool ScriptRenderer::isDimmed(GraphNodePinType type) const
{
	return type.type == int(ScriptNodeElementType::ReadDataPin) || type.type == int(ScriptNodeElementType::TargetPin) || type.type == int(ScriptNodeElementType::WriteDataPin);
}

Colour4f ScriptRenderer::getPinColour(GraphNodePinType pinType) const
{
	switch (ScriptNodeElementType(pinType.type)) {
	case ScriptNodeElementType::FlowPin:
		return pinType.isCancellable ? Colour4f(0.75f, 0.0f, 0.99f) : (pinType.isNetwork ? Colour4f(0.15f, 0.85f, 0.98f) : Colour4f(0.75f, 0.75f, 0.99f));
	case ScriptNodeElementType::ReadDataPin:
		return Colour4f(0.91f, 0.55f, 0.2f);
	case ScriptNodeElementType::WriteDataPin:
		return Colour4f(0.91f, 0.2f, 0.2f);
	case ScriptNodeElementType::TargetPin:
		return Colour4f(0.35f, 1, 0.35f);
	}

	return Colour4f();
}

const IGraphNodeType* ScriptRenderer::tryGetNodeType(const String& typeId) const
{
	return nodeTypeCollection.tryGetNodeType(typeId);
}
