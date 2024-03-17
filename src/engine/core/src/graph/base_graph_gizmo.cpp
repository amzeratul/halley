#include "halley/graph/base_graph_gizmo.h"

#include "halley/editor_extensions/scene_editor_input_state.h"
#include "halley/graph/base_graph.h"
#include "halley/graph/base_graph_renderer.h"
#include "halley/ui/ui_factory.h"
using namespace Halley;

bool BaseGraphGizmo::Connection::operator<(const Connection& other) const
{
	return distance < other.distance;
}

bool BaseGraphGizmo::Connection::conflictsWith(const Connection& other) const
{
	return (srcNode == other.srcNode && srcPin == other.srcPin) || (dstNode == other.dstNode && dstPin == other.dstPin);
}

void BaseGraphGizmo::setBasePosition(Vector2f pos)
{
	basePos = pos;
}

BaseGraphGizmo::BaseGraphGizmo(UIFactory& factory, const IEntityEditorFactory& entityEditorFactory, Resources& resources, float baseZoom)
	: factory(factory)
	, entityEditorFactory(entityEditorFactory)
	, resources(&resources)
	, baseZoom(baseZoom)
{
	tooltipLabel
		.setFont(factory.getResources().get<Font>("Ubuntu Bold"))
		.setSize(14)
		.setColour(Colour(1, 1, 1))
		.setOutlineColour(Colour(0, 0, 0))
		.setOutline(1);
}

void BaseGraphGizmo::update(Time time, const SceneEditorInputState& inputState)
{
	// This must be set before onNodeDragging/onEditingConnection below
	const bool startedIdle = !dragging && !nodeEditingConnection;

	if (dragging) {
		onNodeDragging(inputState);
	} else if (nodeEditingConnection) {
		onEditingConnection(inputState);
	}

	if (!dragging) {
		if (inputState.mousePos) {
			nodeUnderMouse = renderer->getNodeUnderMouse(basePos, getZoom(), *inputState.mousePos, !!nodeEditingConnection);
		} else {
			nodeUnderMouse.reset();
		}
		if (inputState.selectionBox) {
			selectedNodes.updateOrStartDrag(renderer->getNodesInRect(basePos, getZoom(), *inputState.selectionBox), getSelectionModifier(inputState));
		} else {
			selectedNodes.endDrag();
		}
	}

	if (startedIdle && inputState.leftClickPressed && inputState.mousePos) {
		const auto modifier = getSelectionModifier(inputState);
		if (nodeUnderMouse && nodeUnderMouse->element.type == GraphElementType(BaseGraphNodeElementType::Node)) {
			selectedNodes.mouseButtonPressed(nodeUnderMouse->nodeId, modifier, *inputState.mousePos);
		} else {
			selectedNodes.mouseButtonPressed({}, modifier, *inputState.mousePos);
		}
	}
	if (inputState.leftClickReleased && inputState.mousePos) {
		selectedNodes.mouseButtonReleased(*inputState.mousePos);
	}

	if (startedIdle && inputState.mousePos) {
		if (nodeUnderMouse) {
			if (nodeUnderMouse->element.type == GraphElementType(BaseGraphNodeElementType::Node)) {
				if (inputState.leftClickPressed) {
					onNodeClicked(inputState.mousePos.value(), getSelectionModifier(inputState));
				} else if (inputState.rightClickReleased) {
					openNodeUI(nodeUnderMouse->nodeId, inputState.rawMousePos.value(), baseGraph->getNode(nodeUnderMouse->nodeId).getType());
				}
			} else {
				if (inputState.leftClickPressed) {
					onPinClicked(true, inputState.shiftHeld);
				} else if (inputState.rightClickPressed) {
					onPinClicked(false, inputState.shiftHeld);
				}
			}
		}
	}
	
	lastMousePos = inputState.mousePos;
	lastCtrlHeld = inputState.ctrlHeld;
	lastShiftHeld = inputState.shiftHeld;
}

void BaseGraphGizmo::draw(Painter& painter) const
{
	
}

void BaseGraphGizmo::setUIRoot(UIRoot& root)
{
	uiRoot = &root;
}

void BaseGraphGizmo::setEventSink(UIWidget& sink)
{
	eventSink = &sink;
}

void BaseGraphGizmo::setZoom(float zoom)
{
	this->zoom = zoom;
}

float BaseGraphGizmo::getZoom() const
{
	return zoom;
}

void BaseGraphGizmo::setModifiedCallback(ModifiedCallback callback)
{
	modifiedCallback = std::move(callback);
}

void BaseGraphGizmo::onModified()
{
	if (modifiedCallback) {
		modifiedCallback();
	}
}

void BaseGraphGizmo::updateNodeAutoConnection(gsl::span<const GraphNodeId> nodeIds)
{
	pendingAutoConnections.clear();
	
	for (const auto id: nodeIds) {
		const auto& node = baseGraph->getNode(id);
		const size_t nPins = node.getPinConfiguration().size();
		for (size_t pinIdx = 0; pinIdx < nPins; ++pinIdx) {
			const bool empty = !node.getPin(pinIdx).hasConnection();
			const auto srcPinType = node.getPinType(static_cast<GraphPinId>(pinIdx));

			if (empty) {
				auto conn = findAutoConnectionForPin(id, static_cast<GraphPinId>(pinIdx), node.getPosition(), srcPinType, nodeIds);
				if (conn) {
					pendingAutoConnections.push_back(*conn);
				}
			}
		}
	}

	pruneConflictingAutoConnections();
}

void BaseGraphGizmo::pruneConflictingAutoConnections()
{
	std::sort(pendingAutoConnections.begin(), pendingAutoConnections.end());

	for (size_t i = 0; i < pendingAutoConnections.size(); ++i) {
		for (size_t j = i + 1; j < pendingAutoConnections.size(); ++j) {
			if (pendingAutoConnections[i].conflictsWith(pendingAutoConnections[j])) {
				pendingAutoConnections.erase(pendingAutoConnections.begin() + j);
			}
		}
	}
}

bool BaseGraphGizmo::finishAutoConnection()
{
	bool changed = false;
	for (const auto& conn: pendingAutoConnections) {
		changed = baseGraph->connectPins(conn.srcNode, conn.srcPin, conn.dstNode, conn.dstPin) || changed;
	}
	pendingAutoConnections.clear();
	return changed;
}

std::optional<BaseGraphGizmo::Connection> BaseGraphGizmo::findAutoConnectionForPin(GraphNodeId srcNodeId, GraphPinId srcPinIdx, Vector2f nodePos, GraphNodePinType srcPinType, gsl::span<const GraphNodeId> excludeIds) const
{
	float bestDistance = 100.0f;
	std::optional<Connection> bestPath;

	const size_t n  = baseGraph->getNumNodes();
	for (size_t i = 0; i < n; ++i) {
		auto& node = baseGraph->getNode(i);
		if (std_ex::contains(excludeIds, node.getId()))	{
			continue;
		}

		// Coarse distance test
		if ((node.getPosition() - nodePos).length() > 200.0f) {
			continue;
		}

		const size_t nPins = node.getPinConfiguration().size();
		for (size_t pinIdx = 0; pinIdx < nPins; ++pinIdx) {
			const bool empty = !node.getPin(pinIdx).hasConnection();
			const auto dstPinType = node.getPinType(static_cast<GraphPinId>(pinIdx));
			if (empty && srcPinType.canConnectTo(dstPinType)) {
				const auto srcPos = renderer->getPinPosition(basePos, baseGraph->getNode(srcNodeId), srcPinIdx, getZoom());
				const auto dstPos = renderer->getPinPosition(basePos, baseGraph->getNode(node.getId()), static_cast<GraphPinId>(pinIdx), getZoom());
				const float distance = (srcPos - dstPos).length();
				if (distance < bestDistance) {
					bestDistance = distance;
					bestPath = Connection{ srcNodeId, node.getId(), srcPinIdx, static_cast<GraphPinId>(pinIdx), srcPinType, dstPinType, srcPos, dstPos, distance };
				}
			}
		}
	}

	return bestPath;
}

void BaseGraphGizmo::onNodeClicked(Vector2f mousePos, SelectionSetModifier modifier)
{
	if (modifier == SelectionSetModifier::None) {
		Vector<GraphNodeId> nodeIds = selectedNodes.getSelected();
		Vector<Vector2f> startPos;
		startPos.reserve(nodeIds.size());
		for (const auto& node: nodeIds) {
			startPos.push_back(baseGraph->getNode(node).getPosition());
		}
		dragging = Dragging { nodeIds, startPos, mousePos, false };
	}
}

void BaseGraphGizmo::onNodeDragging(const SceneEditorInputState& inputState)
{
	Expects(dragging->nodeIds.size() == dragging->startPos.size());
	if (inputState.mousePos) {
		if (dragging->startMousePos) {
			const Vector2f delta = inputState.mousePos.value() - *dragging->startMousePos;
			if (delta.length() > 1.0f) {
				for (size_t i = 0; i < dragging->nodeIds.size(); ++i) {
					auto& node = baseGraph->getNode(dragging->nodeIds[i]);
					const auto newPos = dragging->startPos[i] + delta;
					node.setPosition(Vector2f(std::floor(newPos.x / gridSize) * gridSize, std::floor(newPos.y / gridSize) * gridSize));
					dragging->hadChange = true;
				}
			}
		} else {
			for (size_t i = 0; i < dragging->nodeIds.size(); ++i) {
				auto& node = baseGraph->getNode(dragging->nodeIds[i]);
				const auto newPos = *inputState.mousePos - basePos;
				node.setPosition(Vector2f(std::floor(newPos.x / gridSize) * gridSize, std::floor(newPos.y / gridSize) * gridSize));
				dragging->hadChange = true;
			}
		}
	}

	if (autoConnectPin) {
		updateNodeAutoConnection(dragging->nodeIds);
	}

	if ((dragging->sticky && inputState.leftClickPressed) || (!dragging->sticky && !inputState.leftClickHeld)) {
		const bool hadChange = dragging->hadChange;
		dragging.reset();
		const bool newConnection = finishAutoConnection();
		if (hadChange || newConnection) {
			onModified();
		}
	}
}

void BaseGraphGizmo::onPinClicked(bool leftClick, bool shiftHeld)
{
	Expects(nodeUnderMouse);

	const auto nodeId = nodeUnderMouse->nodeId;
	const auto pinId = nodeUnderMouse->elementId;
	const auto connections = baseGraph->getPinConnections(nodeId, pinId);
	bool changed = false;
	nodeConnectionDst = {};
	
	if (leftClick) {
		if (shiftHeld) {
			changed = baseGraph->disconnectPinIfSingleConnection(nodeId, pinId);
		} else {
			changed = baseGraph->disconnectPin(nodeId, pinId);
		}

		// If this causes a disconnection and there was exactly one connection before, set anchor THERE, rather than here
		// (so it's like I'm picking up this end of the wire)
		if (changed && connections.size() == 1) {
			const auto other = connections[0];
			nodeEditingConnection = renderer->getPinInfo(basePos, getZoom(), other.first, other.second);
		} else {
			nodeEditingConnection = nodeUnderMouse;
		}
	} else {
		changed = baseGraph->disconnectPin(nodeId, pinId);
	}

	if (changed) {
		onModified();
	}
}

void BaseGraphGizmo::onEditingConnection(const SceneEditorInputState& inputState)
{
	nodeConnectionDst = inputState.mousePos;

	const auto srcType = nodeEditingConnection->element;
	const auto srcNodeId = nodeEditingConnection->nodeId;
	const auto srcPinId = nodeEditingConnection->elementId;

	if (nodeUnderMouse) {
		const auto dstNodeId = nodeUnderMouse->nodeId;
		const auto dstType = nodeUnderMouse->element;

		if (!srcType.canConnectTo(dstType) || srcNodeId == dstNodeId) {
			nodeUnderMouse.reset();
		}
	}

	if (inputState.leftClickPressed) {
		if (nodeUnderMouse) {
			const auto dstNodeId = nodeUnderMouse->nodeId;
			const auto dstPinId = nodeUnderMouse->elementId;
			
			if (baseGraph->connectPins(srcNodeId, srcPinId, dstNodeId, dstPinId)) {
				onModified();
			}
		}
		
		nodeEditingConnection.reset();
	}
	if (inputState.rightClickPressed) {
		nodeEditingConnection.reset();
	}
}

void BaseGraphGizmo::onNodeAdded(GraphNodeId id)
{
}

bool BaseGraphGizmo::isHighlighted() const
{
	return !!nodeUnderMouse || nodeEditingConnection;
}

void BaseGraphGizmo::onMouseWheel(Vector2f mousePos, int amount, KeyMods keyMods)
{
	int axis;
	if (keyMods == KeyMods::Ctrl) {
		axis = 0;
	} else if (keyMods == KeyMods::Shift) {
		axis = 1;
	} else {
		return;
	}

	const float delta = static_cast<float>(amount) * -16.0f;
	const float midPos = (mousePos - basePos)[axis];
	const float mid0 = midPos - 32;
	const float mid1 = midPos + 32;

	const auto n = baseGraph->getNumNodes();
	for (size_t i = 0; i < n; ++i) {
		auto& node = baseGraph->getNode(i);
		auto pos = node.getPosition();
		if (std::abs(pos[axis] - midPos) > 32) {
			pos[axis] = advance(pos[axis], pos[axis] < midPos ? mid0 : mid1, delta);
			node.setPosition(pos);
		}
	}
}

std::optional<BaseGraphRenderer::NodeUnderMouseInfo> BaseGraphGizmo::getNodeUnderMouse() const
{
	return nodeUnderMouse;
}

SelectionSetModifier BaseGraphGizmo::getSelectionModifier(const SceneEditorInputState& inputState) const
{
	if (inputState.ctrlHeld) {
		return SelectionSetModifier::Toggle;
	} else if (inputState.shiftHeld) {
		return SelectionSetModifier::Add;
	} else if (inputState.altHeld) {
		return SelectionSetModifier::Remove;
	} else {
		return SelectionSetModifier::None;
	}
}

void BaseGraphGizmo::addNode()
{
	
}

GraphNodeId BaseGraphGizmo::addNode(const String& type, Vector2f pos, ConfigNode settings)
{
	const auto id = baseGraph->addNode(type, pos, std::move(settings));
	onNodeAdded(id);

	selectedNodes.directSelect(id, SelectionSetModifier::None);
	dragging = Dragging{ { id }, { baseGraph->getNode(id).getPosition() }, {}, true };

	return id;
}

bool BaseGraphGizmo::destroyNode(GraphNodeId id)
{
	return destroyNodes({ id });
}

bool BaseGraphGizmo::destroyNodes(Vector<GraphNodeId> ids)
{
	std::sort(ids.begin(), ids.end(), std::greater<GraphNodeId>());

	bool modified = false;

	for (auto& id: ids) {
		if (nodeUnderMouse && nodeUnderMouse->nodeId == id) {
			nodeUnderMouse.reset();
		}

		if (!canDeleteNode(baseGraph->getNode(id))) {
			continue;
		}
		
		const size_t numNodes = baseGraph->getNumNodes();
		for (size_t i = 0; i < numNodes; ++i) {
			baseGraph->getNode(i).onNodeRemoved(id);
		}

		baseGraph->eraseNode(id);
		modified = true;
	}

	if (modified) {
		onModified();
		baseGraph->finishGraph();
	}

	return modified;
}

bool BaseGraphGizmo::canDeleteNode(const BaseGraphNode& node) const
{
	return true;
}

bool BaseGraphGizmo::nodeTypeNeedsSettings(const String& nodeType) const
{
	return false;
}

void BaseGraphGizmo::openNodeUI(std::optional<GraphNodeId> nodeId, std::optional<Vector2f> pos, const String& nodeType)
{
	if (nodeId || nodeTypeNeedsSettings(nodeType)) {
		openNodeSettings(nodeId, pos, nodeType);
	} else {
		addNode(nodeType, pos.value_or(Vector2f()), ConfigNode::MapType());
	}
}

void BaseGraphGizmo::openNodeSettings(std::optional<GraphNodeId> nodeId, std::optional<Vector2f> pos, const String& nodeType)
{
}
