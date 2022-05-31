#include "scripting_base_gizmo.h"

#include "scripting_choose_node.h"
#include "scripting_gizmo_toolbar.h"
#include "scripting_node_editor.h"
#include "src/scene/choose_window.h"

using namespace Halley;

ScriptingBaseGizmo::ScriptingBaseGizmo(UIFactory& factory, ISceneEditorWindow& sceneEditorWindow, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes, float baseZoom)
	: factory(factory)
	, sceneEditorWindow(sceneEditorWindow)
	, scriptNodeTypes(std::move(scriptNodeTypes))
	, world(&sceneEditorWindow.getEntityFactory()->getWorld())
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

void ScriptingBaseGizmo::update(Time time, Resources& res, const SceneEditorInputState& inputState)
{
	Executor(pendingUITasks).runPending();

	resources = &res;
	
	if (!renderer) {
		renderer = std::make_shared<ScriptRenderer>(res, world, *scriptNodeTypes, baseZoom);
	}
	renderer->setGraph(scriptGraph);

	if (scriptGraph) {
		scriptGraph->assignTypes(*scriptNodeTypes);
	}

	if (!dragging) {
		nodeUnderMouse = renderer->getNodeUnderMouse(basePos, getZoom(), inputState.mousePos, !!nodeEditingConnection);
	}

	// Find entity target under mouse
	curEntityTarget = EntityId();
	const auto curZoom = getZoom();
	if (inputState.mousePos) {
		for (const auto& entity: entityTargets) {
			if (Circle(entity.pos, 6.0f / curZoom).contains(inputState.mousePos.value())) {
				curEntityTarget = entity.entityId;
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

	if (startedIdle && nodeUnderMouse && inputState.mousePos) {
		if (nodeUnderMouse->element.type == ScriptNodeElementType::Node) {
			if (inputState.leftClickPressed) {
				onNodeClicked(inputState.mousePos.value());
			} else if (inputState.rightClickReleased) {
				openNodeUI(nodeUnderMouse->nodeId, inputState.rawMousePos.value(), true);
			}
		} else {
			if (inputState.leftClickPressed) {
				onPinClicked(false, inputState.shiftHeld);
			} else if (inputState.rightClickPressed) {
				onPinClicked(true, inputState.shiftHeld);
			}
		}
	}
	
	lastMousePos = inputState.mousePos;
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

void ScriptingBaseGizmo::onModified()
{
	if (modifiedCallback) {
		modifiedCallback();
	}
}

void ScriptingBaseGizmo::onNodeClicked(Vector2f mousePos)
{
	dragging = true;
	const auto nodePos = scriptGraph->getNodes()[nodeUnderMouse->nodeId].getPosition();
	startDragPos = nodePos - mousePos;
}

void ScriptingBaseGizmo::onNodeDragging(const SceneEditorInputState& inputState)
{
	if (inputState.mousePos) {
		const auto newPos = startDragPos + inputState.mousePos.value();
		scriptGraph->getNodes()[nodeUnderMouse->nodeId].setPosition(newPos);
	}
	if (!inputState.leftClickHeld) {
		dragging = false;
		onModified();
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

		if (srcType.type != dstType.type || srcType.direction == dstType.direction || srcNodeId == dstNodeId) {
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
				if (scriptGraph->connectPin(srcNodeId, srcPinId, curEntityTarget)) {
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
	if (!renderer) {
		return;
	}

	std::optional<ScriptRenderer::ConnectionPath> path;
	if (nodeEditingConnection && nodeConnectionDst) {
		const auto srcType = nodeEditingConnection->element;
		ScriptNodePinType dstType;
		dstType.type = srcType.type;
		dstType.direction = srcType.direction == ScriptNodePinDirection::Input ? ScriptNodePinDirection::Output : ScriptNodePinDirection::Input;
		path = ScriptRenderer::ConnectionPath{ nodeEditingConnection->pinPos, nodeConnectionDst.value(), srcType, dstType };
	}

	drawEntityTargets(painter);
	
	renderer->setHighlight(nodeUnderMouse);
	renderer->setCurrentPath(path);
	renderer->draw(painter, basePos, getZoom());

	if (nodeUnderMouse && !dragging) {
		drawToolTip(painter, scriptGraph->getNodes().at(nodeUnderMouse->nodeId), nodeUnderMouse.value());
	}
}

bool ScriptingBaseGizmo::isHighlighted() const
{
	return !!nodeUnderMouse || curEntityTarget.isValid() || nodeEditingConnection;
}

bool ScriptingBaseGizmo::destroyNode(uint32_t id)
{
	auto& nodes = scriptGraph->getNodes();
	if (id < nodes.size()) {
		const auto* nodeType = scriptNodeTypes->tryGetNodeType(nodes[id].getType());
		if (nodeType && !nodeType->canDelete()) {
			return false;
		}
		
		for (auto& n: nodes) {
			n.onNodeRemoved(id);
		}
		
		nodes.erase(nodes.begin() + id);
		onModified();
		return true;
	}

	return false;
}

bool ScriptingBaseGizmo::destroyHighlightedNode()
{
	if (nodeUnderMouse && nodeUnderMouse->element.type == ScriptNodeElementType::Node) {
		destroyNode(nodeUnderMouse->nodeId);
		nodeUnderMouse.reset();
		return true;
	}
	return false;
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
	dragging = false;
}

ScriptGraphNode& ScriptingBaseGizmo::getNode(uint32_t id)
{
	return scriptGraph->getNodes().at(id);
}

void ScriptingBaseGizmo::setBasePosition(Vector2f pos)
{
	basePos = pos;
}

ExecutionQueue& ScriptingBaseGizmo::getExecutionQueue()
{
	return pendingUITasks;
}

void ScriptingBaseGizmo::drawToolTip(Painter& painter, const ScriptGraphNode& node, const ScriptRenderer::NodeUnderMouseInfo& nodeInfo) const
{
	const auto* nodeType = scriptNodeTypes->tryGetNodeType(node.getType());
	if (!nodeType) {
		return;
	}
	
	const auto [text, colours] = nodeType->getDescription(node, world, nodeInfo.element, nodeInfo.elementId, *scriptGraph);
	const float curZoom = getZoom();

	const float align = 0.5f;
	const auto elemPos = nodeInfo.element.type == ScriptNodeElementType::Node ? 0.5f * (nodeInfo.nodeArea.getBottomLeft() + nodeInfo.nodeArea.getBottomRight()) : nodeInfo.pinPos;
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
		const auto col = e.entityId == curEntityTarget ? Colour4f(1, 1, 1) : Colour4f(0.35f, 1.0f, 0.35f);
		painter.drawCircle(e.pos, radius, width, col);
	}
}

std::shared_ptr<UIWidget> ScriptingBaseGizmo::makeUI()
{
	return std::make_shared<ScriptingGizmoToolbar>(factory, *this);
}

void ScriptingBaseGizmo::openNodeUI(uint32_t nodeId, std::optional<Vector2f> pos, bool force)
{
	ScriptGraphNode& node = getNode(nodeId);
	const auto* nodeType = scriptNodeTypes->tryGetNodeType(node.getType());
	if (nodeType && (force || !nodeType->getSettingTypes().empty())) {
		uiRoot->addChild(std::make_shared<ScriptingNodeEditor>(*this, factory, sceneEditorWindow.getEntityEditorFactory(), nodeId, *nodeType, pos));
	}
}

void ScriptingBaseGizmo::addNode()
{
	const Vector2f pos = lastMousePos ? lastMousePos.value() - basePos : Vector2f();
	
	auto chooseAssetWindow = std::make_shared<ScriptingChooseNode>(Vector2f(), factory, *resources, scriptNodeTypes, [=] (std::optional<String> result)
	{
		if (result) {
			Concurrent::execute(pendingUITasks, [this, type = std::move(result.value()), pos] ()
			{
				addNode(type, pos);
			});
		}
	});
	uiRoot->addChild(std::move(chooseAssetWindow));
}

void ScriptingBaseGizmo::addNode(const String& type, Vector2f pos)
{
	auto& nodes = scriptGraph->getNodes();
	const uint32_t id = static_cast<uint32_t>(nodes.size());
	nodes.emplace_back(type, pos);
	onModified();
	
	openNodeUI(id, {}, false);
}
