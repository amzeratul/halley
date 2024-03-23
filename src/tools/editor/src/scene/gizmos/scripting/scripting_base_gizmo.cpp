#include "scripting_base_gizmo.h"

#include "scripting_choose_node.h"
#include "scripting_gizmo_toolbar.h"
#include "scripting_node_editor.h"
#include "src/scene/choose_window.h"

using namespace Halley;

ScriptingBaseGizmo::ScriptingBaseGizmo(UIFactory& factory, const IEntityEditorFactory& entityEditorFactory, Resources& resources, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes, float baseZoom)
	: BaseGraphGizmo(factory, entityEditorFactory, resources, std::move(scriptNodeTypes), baseZoom)
{
}

void ScriptingBaseGizmo::setEntityTargets(Vector<String> targets)
{
	entityTargets = std::move(targets);
}

std::pair<String, Vector<ColourOverride>> ScriptingBaseGizmo::getNodeDescription(const BaseGraphNode& node, const BaseGraphRenderer::NodeUnderMouseInfo& nodeInfo) const
{
	auto [text, colours] = BaseGraphGizmo::getNodeDescription(node, nodeInfo);

	if (devConData && devConData->first == getNodeUnderMouse() && !devConData->second.isEmpty()) {
		colours.emplace_back(text.size(), std::nullopt);
		text += "\n\nValue: ";
		colours.emplace_back(text.size(), Colour4f(0.44f, 1.0f, 0.94f));
		text += devConData->second;
	}

	return std::pair<String, Vector<ColourOverride>>{ std::move(text), std::move(colours) };
}

ScriptGraph& ScriptingBaseGizmo::getGraph()
{
	return *scriptGraph;
}

void ScriptingBaseGizmo::setGraph(ScriptGraph* graph)
{
	setBaseGraph(graph);
	scriptGraph = graph;
}

void ScriptingBaseGizmo::setState(ScriptState* state)
{
	scriptState = state;
	if (renderer) {
		dynamic_cast<ScriptRenderer&>(*renderer).setState(scriptState);
	}
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

bool ScriptingBaseGizmo::canDeleteNode(const BaseGraphNode& node) const
{
	const auto* nodeType = nodeTypes->tryGetGraphNodeType(node.getType());
	return !nodeType || nodeType->canDelete();
}

bool ScriptingBaseGizmo::nodeTypeNeedsSettings(const String& type) const
{
	const auto* nodeType = nodeTypes->tryGetGraphNodeType(type);
	return nodeType && !nodeType->getSettingTypes().empty();
}

void ScriptingBaseGizmo::openNodeSettings(std::optional<GraphNodeId> nodeId, std::optional<Vector2f> pos, const String& type)
{
	if (const auto* nodeType = nodeTypes->tryGetGraphNodeType(type)) {
		uiRoot->addChild(std::make_shared<ScriptingNodeEditor>(*this, factory, entityEditorFactory, eventSink, nodeId, *nodeType, pos));
	}
}

std::shared_ptr<UIWidget> ScriptingBaseGizmo::makeChooseNodeTypeWindow(Vector2f windowSize, UIFactory& factory, Resources& resources, ChooseAssetWindow::Callback callback)
{
	return std::make_shared<ScriptingChooseNode>(windowSize, factory, resources, nodeTypes, std::move(callback));
}

std::unique_ptr<BaseGraphNode> ScriptingBaseGizmo::makeNode(const ConfigNode& node)
{
	return std::make_unique<ScriptGraphNode>(node);
}

std::shared_ptr<BaseGraphRenderer> ScriptingBaseGizmo::makeRenderer(Resources& resources, float baseZoom)
{
	auto renderer = std::make_shared<ScriptRenderer>(resources, *nodeTypes, baseZoom);
	renderer->setState(scriptState);
	return renderer;
}
