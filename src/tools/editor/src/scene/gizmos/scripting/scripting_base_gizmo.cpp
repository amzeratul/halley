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
	if (!renderer) {
		renderer = std::make_shared<ScriptRenderer>(*resources, world, *scriptNodeTypes, baseZoom);
		dynamic_cast<ScriptRenderer&>(*renderer).setState(scriptState);
	}
	renderer->setGraph(scriptGraph);

	assignNodeTypes();

	BaseGraphGizmo::update(time, inputState);
}

void ScriptingBaseGizmo::draw(Painter& painter) const
{
	assignNodeTypes();

	BaseGraphGizmo::draw(painter);
}

void ScriptingBaseGizmo::setEntityTargets(Vector<String> targets)
{
	entityTargets = std::move(targets);
}

std::pair<String, Vector<ColourOverride>> ScriptingBaseGizmo::getNodeDescription(const BaseGraphNode& node, const BaseGraphRenderer::NodeUnderMouseInfo& nodeInfo) const
{
	const auto* nodeType = scriptNodeTypes->tryGetNodeType(node.getType());
	if (!nodeType) {
		return {};
	}
	
	auto [text, colours] = nodeType->getDescription(dynamic_cast<const ScriptGraphNode&>(node), world, nodeInfo.element, nodeInfo.elementId, *scriptGraph);
	if (devConData && devConData->first == getNodeUnderMouse() && !devConData->second.isEmpty()) {
		colours.emplace_back(text.size(), std::nullopt);
		text += "\n\nValue: ";
		colours.emplace_back(text.size(), Colour4f(0.44f, 1.0f, 0.94f));
		text += devConData->second;
	}

	return std::pair<String, Vector<ColourOverride>>{ std::move(text), std::move(colours) };
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
	resetDrag();
	updateNodes(true);
}

void ScriptingBaseGizmo::setState(ScriptState* state)
{
	scriptState = state;
	if (renderer) {
		dynamic_cast<ScriptRenderer&>(*renderer).setState(scriptState);
	}
}

ScriptGraphNode& ScriptingBaseGizmo::getNode(GraphNodeId id)
{
	return scriptGraph->getNodes().at(id);
}

const ScriptGraphNode& ScriptingBaseGizmo::getNode(GraphNodeId id) const
{
	return scriptGraph->getNodes().at(id);
}

std::shared_ptr<UIWidget> ScriptingBaseGizmo::makeUI()
{
	return std::make_shared<ScriptingGizmoToolbar>(factory, *this);
}

void ScriptingBaseGizmo::setCurNodeDevConData(const String& str)
{
	if (getNodeUnderMouse()) {
		devConData = { *getNodeUnderMouse(), str };
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

std::shared_ptr<UIWidget> ScriptingBaseGizmo::makeChooseNodeTypeWindow(Vector2f windowSize, UIFactory& factory, Resources& resources, ChooseAssetWindow::Callback callback)
{
	return std::make_shared<ScriptingChooseNode>(windowSize, factory, resources, scriptNodeTypes, std::move(callback));
}

std::unique_ptr<BaseGraphNode> ScriptingBaseGizmo::makeNode(const ConfigNode& node)
{
	return std::make_unique<ScriptGraphNode>(node);
}
