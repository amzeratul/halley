#include "halley/scripting/script_renderer.h"
#include "halley/entity/world.h"
#include "halley/graphics/painter.h"
#include "halley/utils/algorithm.h"
#include "halley/scripting/script_graph.h"
#include "halley/scripting/script_node_type.h"
using namespace Halley;

ScriptRenderer::ScriptRenderer(Resources& resources, const ScriptNodeTypeCollection& nodeTypeCollection, float nativeZoom)
	: BaseGraphRenderer(resources, nativeZoom)
	, nodeTypeCollection(nodeTypeCollection)
{
	destructorBg = Sprite().setImage(resources, "halley_ui/script_destructor_bg.png").setPivot(Vector2f(0.5f, 0.0f));
	destructorIcon = Sprite().setImage(resources, "halley_ui/script_destructor_icon.png").setPivot(Vector2f(0.5f, 0.5f));
}

void ScriptRenderer::setState(const ScriptState* scriptState)
{
	state = scriptState;
}

ScriptRenderer::NodeDrawMode ScriptRenderer::getNodeDrawMode(GraphNodeId nodeId) const
{
	NodeDrawMode drawMode = BaseGraphRenderer::getNodeDrawMode(nodeId);
	if (state) {
		const auto nodeIntrospection = state->getNodeIntrospection(nodeId);
		drawMode.activationTime = nodeIntrospection.activationTime < 0.5f ? nodeIntrospection.activationTime * 2 : std::numeric_limits<float>::infinity();
		if (nodeIntrospection.state == ScriptState::NodeIntrospectionState::Active) {
			drawMode.type = NodeDrawModeType::Active;
			drawMode.time = nodeIntrospection.time;
		} else if (nodeIntrospection.state == ScriptState::NodeIntrospectionState::Unvisited) {
			drawMode.type = NodeDrawModeType::Unvisited;
		}
	}
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

std::pair<String, ScriptRenderer::LabelType> ScriptRenderer::getLabel(const IGraphNodeType& nodeType, const BaseGraphNode& node) const
{
	const auto& graphNodeType = dynamic_cast<const IScriptNodeType&>(nodeType);
	const auto classification = graphNodeType.getClassification();
	if (classification == ScriptNodeClassification::DebugDisplay) {
		return { getDebugDisplayValue(node.getId()), LabelType::Large };
	}

	const auto largeLabel = graphNodeType.getLargeLabel(node);
	if (!largeLabel.isEmpty()) {
		return { largeLabel, classification == ScriptNodeClassification::Comment ? LabelType::Comment : LabelType::Large };
	}

	return BaseGraphRenderer::getLabel(nodeType, node);
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

float ScriptRenderer::getIconAlpha(const IGraphNodeType& nodeType, bool dim) const
{
	switch (dynamic_cast<const IScriptNodeType&>(nodeType).getClassification()) {
	case ScriptNodeClassification::Comment:
		return 0.0f;
	case ScriptNodeClassification::DebugDisplay:
		return 0.05f;
	default:
		return BaseGraphRenderer::getIconAlpha(nodeType, dim);
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
