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
	if (devConData && devConData->first == nodeUnderMouse && !devConData->second.isEmpty()) {
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
	dragging.reset();
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

std::shared_ptr<UIWidget> ScriptingBaseGizmo::makeUI()
{
	return std::make_shared<ScriptingGizmoToolbar>(factory, *this);
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

std::shared_ptr<UIWidget> ScriptingBaseGizmo::makeChooseNodeTypeWindow(Vector2f windowSize, UIFactory& factory, Resources& resources, ChooseAssetWindow::Callback callback)
{
	return std::make_shared<ScriptingChooseNode>(windowSize, factory, resources, scriptNodeTypes, std::move(callback));
}
