#include "scripting_base_gizmo.h"

#include "scripting_choose_node.h"
#include "scripting_gizmo_toolbar.h"
#include "scripting_node_editor.h"
#include "src/scene/choose_window.h"

using namespace Halley;

ScriptingBaseGizmo::ScriptingBaseGizmo(UIFactory& factory, const IEntityEditorFactory& entityEditorFactory, const World* world, Resources& resources, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes, float baseZoom)
	: BaseGraphGizmo(factory, entityEditorFactory, resources, baseZoom)
	, scriptNodeTypes(std::move(scriptNodeTypes))
	, world(world)
{
}

void ScriptingBaseGizmo::update(Time time, const SceneEditorInputState& inputState)
{
	Executor(pendingUITasks).runPending();
	
	if (!renderer) {
		renderer = std::make_shared<ScriptRenderer>(*resources, world, *scriptNodeTypes, baseZoom);
	}
	renderer->setGraph(scriptGraph);

	assignNodeTypes();

	BaseGraphGizmo::update(time, inputState);
}

void ScriptingBaseGizmo::setEntityTargets(Vector<String> targets)
{
	entityTargets = std::move(targets);
}


void ScriptingBaseGizmo::draw(Painter& painter) const
{
	drawWheelGuides(painter);

	if (!renderer) {
		return;
	}

	assignNodeTypes();

	Vector<BaseGraphRenderer::ConnectionPath> paths;
	if (nodeEditingConnection && nodeConnectionDst) {
		const auto srcType = nodeEditingConnection->element;
		GraphNodePinType dstType = srcType.getReverseDirection();
		const auto pos = renderer->getPinPosition(basePos, getNode(nodeEditingConnection->nodeId), nodeEditingConnection->elementId, getZoom());
		paths.push_back(BaseGraphRenderer::ConnectionPath{ pos, nodeConnectionDst.value(), srcType, dstType, false });
	}

	for (auto& conn: pendingAutoConnections) {
		paths.push_back(BaseGraphRenderer::ConnectionPath{ conn.srcPos, conn.dstPos, conn.srcType, conn.dstType, true });
	}

	renderer->setHighlight(nodeUnderMouse);
	renderer->setSelection(selectedNodes.getSelected());
	renderer->setCurrentPaths(paths);
	static_cast<ScriptRenderer&>(*renderer).setState(scriptState);
	renderer->draw(painter, basePos, getZoom());

	if (!dragging) {
		if (nodeUnderMouse) {
			drawToolTip(painter, scriptGraph->getNodes().at(nodeUnderMouse->nodeId), nodeUnderMouse.value());
		}
	}
}

void ScriptingBaseGizmo::assignNodeTypes(bool force) const
{
	if (scriptGraph && scriptNodeTypes) {
		scriptGraph->assignTypes(*scriptNodeTypes, force);
	}
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
	baseGraph = graph;
	scriptGraph = graph;
	dragging.reset();
	updateNodes(true);
}

void ScriptingBaseGizmo::setState(ScriptState* state)
{
	scriptState = state;
}

void ScriptingBaseGizmo::setAutoConnectPins(bool autoConnect)
{
	autoConnectPin = autoConnect;
}

ScriptGraphNode& ScriptingBaseGizmo::getNode(GraphNodeId id)
{
	return scriptGraph->getNodes().at(id);
}

const ScriptGraphNode& ScriptingBaseGizmo::getNode(GraphNodeId id) const
{
	return scriptGraph->getNodes().at(id);
}

ConfigNode ScriptingBaseGizmo::copySelection() const
{
	const auto& nodes = scriptGraph->getNodes();
	Vector<ScriptGraphNode> result;
	HashMap<GraphNodeId, GraphNodeId> remap;

	GraphNodeId newIdx = 0;
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
	Vector<GraphNodeId> sel;
	Vector<Vector2f> startPos;
	const auto startNewIdx = static_cast<GraphNodeId>(scriptGraph->getNodes().size());
	auto newIdx = startNewIdx;
	HashMap<GraphNodeId, GraphNodeId> remap;
	for (size_t i = 0; i < nodes.size(); ++i) {
		sel.push_back(newIdx);
		remap[static_cast<GraphNodeId>(i)] = newIdx++;
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

ExecutionQueue& ScriptingBaseGizmo::getExecutionQueue()
{
	return pendingUITasks;
}

void ScriptingBaseGizmo::drawToolTip(Painter& painter, const ScriptGraphNode& node, const BaseGraphRenderer::NodeUnderMouseInfo& nodeInfo) const
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
	const auto elemPos = nodeInfo.element.type == GraphElementType(ScriptNodeElementType::Node) ? 0.5f * (nodeInfo.nodeArea.getBottomLeft() + nodeInfo.nodeArea.getBottomRight()) : nodeInfo.pinPos;
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

std::shared_ptr<UIWidget> ScriptingBaseGizmo::makeUI()
{
	return std::make_shared<ScriptingGizmoToolbar>(factory, *this);
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

void ScriptingBaseGizmo::setCurNodeDevConData(const String& str)
{
	if (nodeUnderMouse) {
		devConData = { *nodeUnderMouse, str };
	} else {
		devConData.reset();
	}
}

void ScriptingBaseGizmo::setDebugDisplayData(HashMap<int, String> values)
{
	renderer->setDebugDisplayData(std::move(values));
}

void ScriptingBaseGizmo::updateNodes(bool force)
{
	if (scriptGraph && resources) {
		assignNodeTypes(force);
		for (auto& node: scriptGraph->getNodes()) {
			node.getNodeType().updateSettings(node, *scriptGraph, *resources);
		}
	}
}

bool ScriptingBaseGizmo::canDeleteNode(const BaseGraphNode& node) const
{
	const auto* nodeType = scriptNodeTypes->tryGetNodeType(node.getType());
	return !nodeType || nodeType->canDelete();
}

bool ScriptingBaseGizmo::nodeTypeNeedsSettings(const String& type) const
{
	const auto* nodeType = scriptNodeTypes->tryGetNodeType(type);
	return nodeType && !nodeType->getSettingTypes().empty();
}

void ScriptingBaseGizmo::openNodeSettings(std::optional<GraphNodeId> nodeId, std::optional<Vector2f> pos, const String& type)
{
	if (const auto* nodeType = scriptNodeTypes->tryGetNodeType(type)) {
		uiRoot->addChild(std::make_shared<ScriptingNodeEditor>(*this, factory, entityEditorFactory, eventSink, nodeId, *nodeType, pos));
	}
}

void ScriptingBaseGizmo::onNodeAdded(GraphNodeId id)
{
	assignNodeTypes();
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
