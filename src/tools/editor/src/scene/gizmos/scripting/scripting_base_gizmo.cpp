#include "scripting_base_gizmo.h"

#include "scripting_choose_node.h"
#include "scripting_gizmo_toolbar.h"
#include "scripting_node_editor.h"
#include "src/scene/choose_window.h"

using namespace Halley;

ScriptingBaseGizmo::ScriptingBaseGizmo(UIFactory& factory, const IEntityEditorFactory& entityEditorFactory, const World* world, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes, float baseZoom)
	: factory(factory)
	, entityEditorFactory(entityEditorFactory)
	, scriptNodeTypes(std::move(scriptNodeTypes))
	, world(world)
	, baseZoom(baseZoom)
{
	tooltipLabel
		.setFont(factory.getResources().get<Font>("Ubuntu Bold"))
		.setSize(14)
		.setColour(Colour(1, 1, 1))
		.setOutlineColour(Colour(0, 0, 0))
		.setOutline(1);
}

void ScriptingBaseGizmo::setUIRoot(UIRoot& root)
{
	uiRoot = &root;
}

void ScriptingBaseGizmo::setEventSink(UIWidget& sink)
{
	eventSink = &sink;
}

void ScriptingBaseGizmo::update(Time time, Resources& res, const SceneEditorInputState& inputState)
{
	Executor(pendingUITasks).runPending();

	if (resources != &res) {
		resources = &res;
		updateNodes();
	}
	
	if (!renderer) {
		renderer = std::make_shared<ScriptRenderer>(res, world, *scriptNodeTypes, baseZoom);
	}
	renderer->setGraph(scriptGraph);

	assignNodeTypes();

	// Find entity target under mouse
	curEntityTarget.reset();
	const auto curZoom = getZoom();
	if (inputState.mousePos && !inputState.selectionBox) {
		for (size_t i = 0; i < entityTargets.size(); ++i) {
			const auto& entity = entityTargets[i];
			if (Circle(entity.pos, 6.0f / curZoom).contains(inputState.mousePos.value())) {
				curEntityTarget = i;
			}
		}
	}

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
		if (nodeUnderMouse && nodeUnderMouse->element.type == ScriptNodeElementType::Node) {
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
			if (nodeUnderMouse->element.type == ScriptNodeElementType::Node) {
				if (inputState.leftClickPressed) {
					onNodeClicked(inputState.mousePos.value(), getSelectionModifier(inputState));
				} else if (inputState.rightClickReleased) {
					openNodeUI(nodeUnderMouse->nodeId, inputState.rawMousePos.value(), getNode(nodeUnderMouse->nodeId).getType());
				}
			} else {
				if (inputState.leftClickPressed) {
					onPinClicked(false, inputState.shiftHeld);
				} else if (inputState.rightClickPressed) {
					onPinClicked(true, inputState.shiftHeld);
				}
			}
		}
	}
	
	lastMousePos = inputState.mousePos;
	lastCtrlHeld = inputState.ctrlHeld;
	lastShiftHeld = inputState.shiftHeld;
}

void ScriptingBaseGizmo::setZoom(float zoom)
{
	this->zoom = zoom;
}

float ScriptingBaseGizmo::getZoom() const
{
	return zoom;
}

void ScriptingBaseGizmo::setModifiedCallback(ModifiedCallback callback)
{
	modifiedCallback = std::move(callback);
}

void ScriptingBaseGizmo::setEntityTargets(Vector<EntityTarget> targets)
{
	entityTargets = std::move(targets);
}

bool ScriptingBaseGizmo::Connection::operator<(const Connection& other) const
{
	return distance < other.distance;
}

bool ScriptingBaseGizmo::Connection::conflictsWith(const Connection& other) const
{
	return (srcNode == other.srcNode && srcPin == other.srcPin) || (dstNode == other.dstNode && dstPin == other.dstPin);
}

void ScriptingBaseGizmo::onModified()
{
	if (modifiedCallback) {
		modifiedCallback();
	}
}

void ScriptingBaseGizmo::onNodeClicked(Vector2f mousePos, SelectionSetModifier modifier)
{
	if (modifier == SelectionSetModifier::None) {
		const auto& nodes = scriptGraph->getNodes();
		Vector<ScriptNodeId> nodeIds = selectedNodes.getSelected();
		Vector<Vector2f> startPos;
		startPos.reserve(nodeIds.size());
		for (auto& node: nodeIds) {
			startPos.push_back(nodes[node].getPosition());
		}
		dragging = Dragging { nodeIds, startPos, mousePos, false };
	}
}

void ScriptingBaseGizmo::onNodeDragging(const SceneEditorInputState& inputState)
{
	Expects(dragging->nodeIds.size() == dragging->startPos.size());
	if (inputState.mousePos) {
		if (dragging->startMousePos) {
			const Vector2f delta = inputState.mousePos.value() - *dragging->startMousePos;
			if (delta.length() > 1.0f) {
				for (size_t i = 0; i < dragging->nodeIds.size(); ++i) {
					auto& node = scriptGraph->getNodes()[dragging->nodeIds[i]];
					node.setPosition(dragging->startPos[i] + delta);
					dragging->hadChange = true;
				}
			}
		} else {
			for (size_t i = 0; i < dragging->nodeIds.size(); ++i) {
				auto& node = scriptGraph->getNodes()[dragging->nodeIds[i]];
				node.setPosition(*inputState.mousePos - basePos);
				dragging->hadChange = true;
			}
		}
	}

	updateNodeAutoConnection(dragging->nodeIds);

	if ((dragging->sticky && inputState.leftClickPressed) || (!dragging->sticky && !inputState.leftClickHeld)) {
		const bool hadChange = dragging->hadChange;
		dragging.reset();
		const bool newConnection = finishAutoConnection();
		if (hadChange || newConnection) {
			onModified();
		}
	}
}

void ScriptingBaseGizmo::onPinClicked(bool rightClick, bool shiftHeld)
{
	Expects(nodeUnderMouse);
	
	if (!rightClick) {
		nodeEditingConnection = nodeUnderMouse;
	}

	const bool changed = rightClick || !shiftHeld ?
		scriptGraph->disconnectPin(nodeUnderMouse->nodeId, nodeUnderMouse->elementId) :
		scriptGraph->disconnectPinIfSingleConnection(nodeUnderMouse->nodeId, nodeUnderMouse->elementId);
	
	if (changed) {
		onModified();
	}
}

void ScriptingBaseGizmo::onEditingConnection(const SceneEditorInputState& inputState)
{
	using ET = ScriptNodeElementType;
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
			
			if (scriptGraph->connectPins(srcNodeId, srcPinId, dstNodeId, dstPinId)) {
				onModified();
			}
		} else {
			if (srcType.type == ET::TargetPin && srcType.direction == ScriptNodePinDirection::Input) {
				if (scriptGraph->connectPin(srcNodeId, srcPinId, curEntityTarget ? entityTargets[*curEntityTarget].entityId : EntityId())) {
					onModified();
				}
			}
		}
		
		nodeEditingConnection.reset();
	}
	if (inputState.rightClickPressed) {
		nodeEditingConnection.reset();
	}
}

void ScriptingBaseGizmo::draw(Painter& painter) const
{
	drawWheelGuides(painter);

	if (!renderer) {
		return;
	}

	assignNodeTypes();

	Vector<ScriptRenderer::ConnectionPath> paths;
	if (nodeEditingConnection && nodeConnectionDst) {
		const auto srcType = nodeEditingConnection->element;
		ScriptNodePinType dstType = srcType.getReverseDirection();
		paths.push_back(ScriptRenderer::ConnectionPath{ nodeEditingConnection->pinPos, nodeConnectionDst.value(), srcType, dstType, false });
	}

	for (auto& conn: pendingAutoConnections) {
		paths.push_back(ScriptRenderer::ConnectionPath{ conn.srcPos, conn.dstPos, conn.srcType, conn.dstType, true });
	}

	drawEntityTargets(painter);
	
	renderer->setHighlight(nodeUnderMouse, curEntityTarget ? scriptGraph->getEntityIdx(entityTargets[*curEntityTarget].entityId) : std::nullopt);
	renderer->setSelection(selectedNodes.getSelected());
	renderer->setCurrentPaths(paths);
	renderer->setState(scriptState);
	renderer->draw(painter, basePos, getZoom());

	if (!dragging) {
		if (nodeUnderMouse) {
			drawToolTip(painter, scriptGraph->getNodes().at(nodeUnderMouse->nodeId), nodeUnderMouse.value());
		} else if (curEntityTarget) {
			drawToolTip(painter, entityTargets[curEntityTarget.value()]);
		}
	}
}

void ScriptingBaseGizmo::assignNodeTypes() const
{
	if (scriptGraph && scriptNodeTypes) {
		scriptGraph->assignTypes(*scriptNodeTypes);
	}
}

SelectionSetModifier ScriptingBaseGizmo::getSelectionModifier(const SceneEditorInputState& inputState) const
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

bool ScriptingBaseGizmo::isHighlighted() const
{
	return !!nodeUnderMouse || curEntityTarget || nodeEditingConnection;
}

bool ScriptingBaseGizmo::destroyNode(ScriptNodeId id)
{
	return destroyNodes({ id });
}

bool ScriptingBaseGizmo::destroyNodes(Vector<ScriptNodeId> ids)
{
	std::sort(ids.begin(), ids.end(), std::greater<ScriptNodeId>());

	bool modified = false;

	auto& nodes = scriptGraph->getNodes();
	for (auto& id: ids) {
		if (nodeUnderMouse && nodeUnderMouse->nodeId == id) {
			nodeUnderMouse.reset();
		}

		const auto* nodeType = scriptNodeTypes->tryGetNodeType(nodes[id].getType());
		if (nodeType && !nodeType->canDelete()) {
			continue;
		}
		
		for (auto& n: nodes) {
			n.onNodeRemoved(id);
		}
		
		nodes.erase(nodes.begin() + id);
		modified = true;
	}

	if (modified) {
		onModified();
		scriptGraph->finishGraph();
	}

	return modified;
}

ScriptGraph& ScriptingBaseGizmo::getGraph()
{
	return *scriptGraph;
}

ScriptGraph* ScriptingBaseGizmo::getGraphPtr()
{
	return scriptGraph;
}

void ScriptingBaseGizmo::setGraph(ScriptGraph* graph)
{
	scriptGraph = graph;
	dragging.reset();
	updateNodes();
}

void ScriptingBaseGizmo::setState(ScriptState* state)
{
	scriptState = state;
}

ScriptGraphNode& ScriptingBaseGizmo::getNode(ScriptNodeId id)
{
	return scriptGraph->getNodes().at(id);
}

const ScriptGraphNode& ScriptingBaseGizmo::getNode(ScriptNodeId id) const
{
	return scriptGraph->getNodes().at(id);
}

ConfigNode ScriptingBaseGizmo::copySelection() const
{
	const auto& nodes = scriptGraph->getNodes();
	Vector<ScriptGraphNode> result;
	HashMap<ScriptNodeId, ScriptNodeId> remap;

	ScriptNodeId newIdx = 0;
	for (const auto id: selectedNodes.getSelected()) {
		remap[id] = newIdx++;
	}

	for (const auto id: selectedNodes.getSelected()) {
		auto& node = result.emplace_back(nodes[id]);
		node.remapNodes(remap);
	}

	return ConfigNode(result);
}

void ScriptingBaseGizmo::paste(const ConfigNode& node)
{
	if (!isValidPaste(node)) {
		return;
	}
	Vector<ScriptGraphNode> nodes = node.asVector<ScriptGraphNode>();
	if (nodes.empty()) {
		return;
	}

	// Find centre position
	Vector2f avgPos;
	for (auto& n: nodes) {
		avgPos += n.getPosition();
	}
	avgPos /= static_cast<float>(nodes.size());
	Vector2f offset;
	if (lastMousePos) {
		offset = lastMousePos.value() - basePos - avgPos;
	}

	// Reassign ids and paste
	Vector<ScriptNodeId> sel;
	Vector<Vector2f> startPos;
	const auto startNewIdx = static_cast<ScriptNodeId>(scriptGraph->getNodes().size());
	auto newIdx = startNewIdx;
	HashMap<ScriptNodeId, ScriptNodeId> remap;
	for (size_t i = 0; i < nodes.size(); ++i) {
		sel.push_back(newIdx);
		remap[static_cast<ScriptNodeId>(i)] = newIdx++;
	}
	for (size_t i = 0; i < nodes.size(); ++i) {
		auto& node = scriptGraph->getNodes().emplace_back(nodes[i]);
		node.remapNodes(remap);

		const auto pos = node.getPosition() + offset;
		node.setPosition(pos);
		startPos.push_back(pos);
	}

	// Update selection
	selectedNodes.setSelection(sel);
	dragging = Dragging{ std::move(sel), std::move(startPos), lastMousePos, true };

	onModified();
	scriptGraph->finishGraph();
}

bool ScriptingBaseGizmo::isValidPaste(const ConfigNode& node) const
{
	if (node.getType() != ConfigNodeType::Sequence) {
		return false;
	}
	for (const auto& n: node.asSequence()) {
		if (n.getType() != ConfigNodeType::Map) {
			return false;
		}
		if (!n.hasKey("position") || !n.hasKey("pins") || !n.hasKey("type")) {
			return false;
		}
	}
	return true;
}

void ScriptingBaseGizmo::copySelectionToClipboard(const std::shared_ptr<IClipboard>& clipboard) const
{
	const auto sel = copySelection();

	YAMLConvert::EmitOptions options;
	options.mapKeyOrder = { "type", "settings", "position", "pins" };
	options.compactMaps = true;
	clipboard->setData(YAMLConvert::generateYAML(sel, options));
}

void ScriptingBaseGizmo::pasteFromClipboard(const std::shared_ptr<IClipboard>& clipboard)
{
	auto strData = clipboard->getStringData();
	if (strData) {
		try {
			const ConfigNode node = YAMLConvert::parseConfig(strData.value());
			paste(node);
		} catch (...) {}
	}
}

void ScriptingBaseGizmo::cutSelectionToClipboard(const std::shared_ptr<IClipboard>& clipboard)
{
	copySelectionToClipboard(clipboard);
	deleteSelection();
}

ConfigNode ScriptingBaseGizmo::cutSelection()
{
	auto result = copySelection();
	deleteSelection();
	return result;
}

bool ScriptingBaseGizmo::deleteSelection()
{
	const bool changed = destroyNodes(selectedNodes.getSelected());
	selectedNodes.clear();
	dragging.reset();
	return changed;
}

void ScriptingBaseGizmo::setBasePosition(Vector2f pos)
{
	basePos = pos;
}

ExecutionQueue& ScriptingBaseGizmo::getExecutionQueue()
{
	return pendingUITasks;
}

void ScriptingBaseGizmo::drawToolTip(Painter& painter, const EntityTarget& entityTarget) const
{
	ColourStringBuilder builder;
	builder.append("Entity ");
	builder.append("\"" + world->getEntity(entityTarget.entityId).getName() + "\"", IScriptNodeType::parameterColour);
	const auto [text, colours] = builder.moveResults();
	drawToolTip(painter, text, colours, entityTarget.pos + Vector2f(0, 10));
}

void ScriptingBaseGizmo::drawToolTip(Painter& painter, const ScriptGraphNode& node, const ScriptRenderer::NodeUnderMouseInfo& nodeInfo) const
{
	const auto* nodeType = scriptNodeTypes->tryGetNodeType(node.getType());
	if (!nodeType) {
		return;
	}
	
	auto [text, colours] = nodeType->getDescription(node, world, nodeInfo.element, nodeInfo.elementId, *scriptGraph);
	if (devConData && devConData->first == nodeUnderMouse && !devConData->second.isEmpty()) {
		colours.emplace_back(text.size(), std::nullopt);
		text += "\n\nValue: ";
		colours.emplace_back(text.size(), Colour4f(0.44f, 1.0f, 0.94f));
		text += devConData->second;
	}
	const auto elemPos = nodeInfo.element.type == ScriptNodeElementType::Node ? 0.5f * (nodeInfo.nodeArea.getBottomLeft() + nodeInfo.nodeArea.getBottomRight()) : nodeInfo.pinPos;
	drawToolTip(painter, text, colours, elemPos);
}

void ScriptingBaseGizmo::drawToolTip(Painter& painter, const String& text, const Vector<ColourOverride>& colours, Vector2f elemPos) const
{
	const float align = 0.5f;
	const float curZoom = getZoom();
	const auto pos = elemPos + Vector2f(0, 10) / curZoom;
	
	tooltipLabel
		.setColourOverride(colours)
		.setPosition(pos)
		.setAlignment(align)
		.setSize(16 / curZoom)
		.setOutline(4.0f / curZoom);

	tooltipLabel
		.setText(tooltipLabel.split(text, 250.0f / curZoom));

	const auto extents = tooltipLabel.getExtents();
	const Rect4f tooltipArea = Rect4f(pos + extents * Vector2f(-align, 0), pos + extents * Vector2f(1.0f - align, 1.0f)).grow(4 / curZoom, 2 / curZoom, 4 / curZoom, 4 / curZoom);
	const auto poly = Polygon({ tooltipArea.getTopLeft(), tooltipArea.getTopRight(), tooltipArea.getBottomRight(), tooltipArea.getBottomLeft() });
	painter.drawPolygon(poly, Colour4f(0, 0, 0, 0.6f));

	tooltipLabel
		.draw(painter);
}

void ScriptingBaseGizmo::drawEntityTargets(Painter& painter) const
{
	const float curZoom = getZoom();
	
	for (const auto& e: entityTargets) {
		const float radius = 6.0f / curZoom;
		const float width = 1.5f / curZoom;
		const auto col = curEntityTarget && entityTargets[*curEntityTarget].entityId == e.entityId ? Colour4f(1, 1, 1) : Colour4f(0.35f, 1.0f, 0.35f);
		painter.drawCircle(e.pos, radius, width, col);
	}
}

std::shared_ptr<UIWidget> ScriptingBaseGizmo::makeUI()
{
	return std::make_shared<ScriptingGizmoToolbar>(factory, *this);
}

void ScriptingBaseGizmo::openNodeUI(std::optional<ScriptNodeId> nodeId, std::optional<Vector2f> pos, const String& type)
{
	const auto* nodeType = scriptNodeTypes->tryGetNodeType(type);
	if (nodeType && (nodeId || !nodeType->getSettingTypes().empty())) {
		uiRoot->addChild(std::make_shared<ScriptingNodeEditor>(*this, factory, entityEditorFactory, eventSink, nodeId, *nodeType, pos));
	} else if (!nodeId) {
		addNode(type, pos.value_or(Vector2f()), ConfigNode());
	}
}

void ScriptingBaseGizmo::addNode()
{
	if (uiRoot->hasModalUI()) {
		return;
	}

	const auto windowSize = uiRoot->getRect().getSize() - Vector2f(900, 350);

	auto chooseAssetWindow = std::make_shared<ScriptingChooseNode>(windowSize, factory, *resources, scriptNodeTypes, [=] (std::optional<String> result)
	{
		if (result) {
			Concurrent::execute(pendingUITasks, [this, type = std::move(result.value())] ()
			{
				openNodeUI({}, {}, type);
			});
		}
	});
	uiRoot->addChild(std::move(chooseAssetWindow));
}

ScriptNodeId ScriptingBaseGizmo::addNode(const String& type, Vector2f pos, ConfigNode settings)
{
	auto& nodes = scriptGraph->getNodes();
	const ScriptNodeId id = static_cast<ScriptNodeId>(nodes.size());
	nodes.emplace_back(type, pos);
	nodes.back().getSettings() = std::move(settings);
	scriptGraph->finishGraph();
	assignNodeTypes();
	onModified();

	selectedNodes.directSelect(id, SelectionSetModifier::None);
	dragging = Dragging{ { id }, { scriptGraph->getNodes()[id].getPosition() }, {}, true };

	return id;
}

void ScriptingBaseGizmo::updateNodeAutoConnection(gsl::span<const ScriptNodeId> nodeIds)
{
	pendingAutoConnections.clear();

	const auto& nodes = scriptGraph->getNodes();
	for (const auto id: nodeIds) {
		const auto& node = nodes[id];
		const size_t nPins = node.getNodeType().getPinConfiguration(node).size();
		for (size_t pinIdx = 0; pinIdx < nPins; ++pinIdx) {
			const bool empty = !node.getPin(pinIdx).hasConnection();
			const auto srcPinType = node.getPinType(static_cast<ScriptPinId>(pinIdx));

			if (empty) {
				auto conn = findAutoConnectionForPin(id, static_cast<ScriptPinId>(pinIdx), node.getPosition(), srcPinType, nodeIds);
				if (conn) {
					pendingAutoConnections.push_back(*conn);
				}
			}
		}
	}

	pruneConflictingAutoConnections();
}

void ScriptingBaseGizmo::pruneConflictingAutoConnections()
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

bool ScriptingBaseGizmo::finishAutoConnection()
{
	bool changed = false;
	for (const auto& conn: pendingAutoConnections) {
		changed = scriptGraph->connectPins(conn.srcNode, conn.srcPin, conn.dstNode, conn.dstPin) || changed;
	}
	pendingAutoConnections.clear();
	return changed;
}

std::optional<ScriptingBaseGizmo::Connection> ScriptingBaseGizmo::findAutoConnectionForPin(ScriptNodeId srcNodeId, ScriptPinId srcPinIdx, Vector2f nodePos, ScriptNodePinType srcPinType, gsl::span<const ScriptNodeId> excludeIds) const
{
	float bestDistance = 100.0f;
	std::optional<Connection> bestPath;

	for (auto& node: scriptGraph->getNodes()) {
		if (std_ex::contains(excludeIds, node.getId()))	{
			continue;
		}

		// Coarse distance test
		if ((node.getPosition() - nodePos).length() > 200.0f) {
			continue;
		}

		const size_t nPins = node.getNodeType().getPinConfiguration(node).size();
		for (size_t pinIdx = 0; pinIdx < nPins; ++pinIdx) {
			const bool empty = !node.getPin(pinIdx).hasConnection();
			const auto dstPinType = node.getPinType(static_cast<ScriptPinId>(pinIdx));
			if (empty && srcPinType.canConnectTo(dstPinType)) {
				const auto srcPos = renderer->getPinPosition(basePos, getNode(srcNodeId), srcPinIdx, getZoom());
				const auto dstPos = renderer->getPinPosition(basePos, getNode(node.getId()), static_cast<ScriptPinId>(pinIdx), getZoom());
				const float distance = (srcPos - dstPos).length();
				if (distance < bestDistance) {
					bestDistance = distance;
					bestPath = Connection{ srcNodeId, node.getId(), srcPinIdx, static_cast<ScriptPinId>(pinIdx), srcPinType, dstPinType, srcPos, dstPos, distance };
				}
			}
		}
	}

	return bestPath;
}

void ScriptingBaseGizmo::onMouseWheel(Vector2f mousePos, int amount, KeyMods keyMods)
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

	for (auto& node: scriptGraph->getNodes()) {
		auto pos = node.getPosition();
		if (std::abs(pos[axis] - midPos) > 32) {
			pos[axis] = advance(pos[axis], pos[axis] < midPos ? mid0 : mid1, delta);
			node.setPosition(pos);
		}
	}
}

std::optional<ScriptRenderer::NodeUnderMouseInfo> ScriptingBaseGizmo::getNodeUnderMouse() const
{
	return nodeUnderMouse;
}

void ScriptingBaseGizmo::setCurNodeDevConData(const String& str)
{
	if (nodeUnderMouse) {
		devConData = { *nodeUnderMouse, str };
	} else {
		devConData.reset();
	}
}

void ScriptingBaseGizmo::updateNodes()
{
	if (scriptGraph && resources) {
		assignNodeTypes();
		for (auto& node: scriptGraph->getNodes()) {
			node.getNodeType().updateSettings(node, *scriptGraph, *resources);
		}
	}
}

void ScriptingBaseGizmo::drawWheelGuides(Painter& painter) const
{
	const auto viewPort = Rect4f(painter.getViewPort());

	if (lastMousePos) {
		if (lastCtrlHeld) {
			painter.drawLine(Vector<Vector2f>{ Vector2f(lastMousePos->x, viewPort.getTop()), Vector2f(lastMousePos->x, viewPort.getBottom()) }, 1, Colour4f(1, 1, 1, 1));
		} else if (lastShiftHeld) {
			painter.drawLine(Vector<Vector2f>{ Vector2f(viewPort.getLeft(), lastMousePos->y), Vector2f(viewPort.getRight(), lastMousePos->y) }, 1, Colour4f(1, 1, 1, 1));
		}
	}
}
