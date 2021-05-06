#include "scripting_gizmo.h"
#include <components/script_component.h>
#include <components/script_target_component.h>

#include "halley/entity/components/transform_2d_component.h"
#include "src/scene/choose_asset_window.h"

using namespace Halley;

ScriptingGizmo::ScriptingGizmo(SnapRules snapRules, UIFactory& factory, ISceneEditorWindow& sceneEditorWindow, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes)
	: SceneEditorGizmo(snapRules)
	, factory(factory)
	, sceneEditorWindow(sceneEditorWindow)
	, scriptNodeTypes(std::move(scriptNodeTypes))
{
	tooltipLabel
		.setFont(factory.getResources().get<Font>("Ubuntu Bold"))
		.setSize(14)
		.setColour(Colour(1, 1, 1))
		.setOutlineColour(Colour(0, 0, 0))
		.setOutline(1);

	compileEntityTargetList();
}

void ScriptingGizmo::update(Time time, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState)
{
	Executor(pendingUITasks).runPending();
	
	if (!renderer) {
		renderer = std::make_shared<ScriptRenderer>(sceneEditor.getResources(), sceneEditor.getWorld(), *scriptNodeTypes, sceneEditorWindow.getProjectDefaultZoom());
	}
	renderer->setGraph(scriptGraph);

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
				onPinClicked();
			}
		}
	}
	
	lastMousePos = inputState.mousePos;
}

void ScriptingGizmo::onNodeClicked(Vector2f mousePos)
{
	dragging = true;
	const auto nodePos = scriptGraph->getNodes()[nodeUnderMouse->nodeId].getPosition();
	startDragPos = nodePos - mousePos;
}

void ScriptingGizmo::onNodeDragging(const SceneEditorInputState& inputState)
{
	if (inputState.mousePos) {
		const auto newPos = startDragPos + inputState.mousePos.value();
		scriptGraph->getNodes()[nodeUnderMouse->nodeId].setPosition(newPos);
	}
	if (!inputState.leftClickHeld) {
		dragging = false;
		saveEntityData();
	}
}

void ScriptingGizmo::onPinClicked()
{
	Expects(nodeUnderMouse);
	
	nodeEditingConnection = nodeUnderMouse;
	
	if (scriptGraph->disconnectPin(nodeEditingConnection->nodeId, nodeEditingConnection->elementId)) {
		saveEntityData();
	}
}

void ScriptingGizmo::onEditingConnection(const SceneEditorInputState& inputState)
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
				saveEntityData();
			}
		}
		
		if (srcType.type == ET::TargetPin) {
			if (scriptGraph->connectPin(srcNodeId, srcPinId, curEntityTarget)) {
				saveEntityData();
			}
		}
		
		nodeEditingConnection.reset();
	}
	if (inputState.rightClickPressed) {
		nodeEditingConnection.reset();
	}
}

void ScriptingGizmo::compileEntityTargetList()
{
	entityTargets.clear();
	const auto& world = sceneEditorWindow.getEntityFactory()->getWorld();
	for (const auto& e: world.getEntities()) {
		if (e.hasComponent<ScriptTargetComponent>()) {
			const auto pos = e.getComponent<Transform2DComponent>().getGlobalPosition();
			entityTargets.emplace_back(EntityTarget{ pos, e.getEntityId() });
		}
	}
}

void ScriptingGizmo::draw(Painter& painter) const
{
	if (!renderer) {
		return;
	}

	std::optional<ScriptRenderer::ConnectionPath> path;
	if (nodeEditingConnection && nodeConnectionDst) {
		const auto srcType = nodeEditingConnection->element;
		ScriptNodePinType dstType;
		dstType.type = srcType.type == ScriptNodeElementType::TargetPin ? ScriptNodeElementType::Undefined : srcType.type;
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

bool ScriptingGizmo::isHighlighted() const
{
	return !!nodeUnderMouse || curEntityTarget.isValid() || nodeEditingConnection;
}

std::vector<String> ScriptingGizmo::getHighlightedComponents() const
{
	return { "Script" };
}

void ScriptingGizmo::refreshEntity()
{
	loadEntityData();
}

void ScriptingGizmo::onEntityChanged()
{
	loadEntityData();
}

void ScriptingGizmo::loadEntityData()
{
	const auto* transform = getComponent<Transform2DComponent>();
	basePos = transform ? transform->getGlobalPosition() : Vector2f();

	auto* script = getComponent<ScriptComponent>();
	scriptGraph = script ? &script->scriptGraph : nullptr;

	dragging = false;
}

void ScriptingGizmo::saveEntityData()
{
	ConfigNode scriptGraphData;
	if (scriptGraph) {
		const auto context = sceneEditorWindow.getEntityFactory()->makeStandaloneContext();
		scriptGraphData = scriptGraph->toConfigNode(context->getConfigNodeContext());
	}
	
	auto* data = getComponentData("Script");
	if (data) {
		(*data)["scriptGraph"] = scriptGraphData;
	}
	markModified("Script", "scriptGraph");
}

bool ScriptingGizmo::destroyNode(uint32_t id)
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
		saveEntityData();
		return true;
	}

	return false;
}

bool ScriptingGizmo::destroyHighlightedNode()
{
	if (nodeUnderMouse && nodeUnderMouse->element.type == ScriptNodeElementType::Node) {
		destroyNode(nodeUnderMouse->nodeId);
		nodeUnderMouse.reset();
		return true;
	}
	return false;
}

ScriptGraphNode& ScriptingGizmo::getNode(uint32_t id)
{
	return scriptGraph->getNodes().at(id);
}

ExecutionQueue& ScriptingGizmo::getExecutionQueue()
{
	return pendingUITasks;
}

void ScriptingGizmo::drawToolTip(Painter& painter, const ScriptGraphNode& node, const ScriptRenderer::NodeUnderMouseInfo& nodeInfo) const
{
	const auto* nodeType = scriptNodeTypes->tryGetNodeType(node.getType());
	if (!nodeType) {
		return;
	}
	
	const auto [text, colours] = nodeType->getDescription(node, sceneEditorWindow.getEntityFactory()->getWorld(), nodeInfo.element, nodeInfo.elementId);
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

void ScriptingGizmo::drawEntityTargets(Painter& painter) const
{
	const float curZoom = getZoom();
	
	for (const auto& e: entityTargets) {
		const float radius = 6.0f / curZoom;
		const float width = 1.5f / curZoom;
		const auto col = e.entityId == curEntityTarget ? Colour4f(1, 1, 1) : Colour4f(0.35f, 1.0f, 0.35f);
		painter.drawCircle(e.pos, radius, width, col);
	}
}

std::shared_ptr<UIWidget> ScriptingGizmo::makeUI()
{
	return std::make_shared<ScriptingGizmoToolbar>(factory, *this);
}

void ScriptingGizmo::openNodeUI(uint32_t nodeId, std::optional<Vector2f> pos, bool force)
{
	ScriptGraphNode& node = getNode(nodeId);
	const auto* nodeType = scriptNodeTypes->tryGetNodeType(node.getType());
	if (nodeType && (force || !nodeType->getSettingTypes().empty())) {
		sceneEditorWindow.spawnUI(std::make_shared<ScriptingNodeEditor>(*this, factory, sceneEditorWindow.getEntityEditorFactory(), nodeId, *nodeType, pos));
	}
}

void ScriptingGizmo::addNode()
{
	const Vector2f pos = lastMousePos ? lastMousePos.value() - basePos : Vector2f();
	
	auto chooseAssetWindow = std::make_shared<ChooseAssetWindow>(factory, [=] (std::optional<String> result)
	{
		if (result) {
			Concurrent::execute(pendingUITasks, [this, type = std::move(result.value()), pos] ()
			{
				addNode(type, pos);
			});
		}
	}, false);
	chooseAssetWindow->setAssetIds(scriptNodeTypes->getTypes(false), scriptNodeTypes->getNames(false), "");
	chooseAssetWindow->setTitle(LocalisedString::fromHardcodedString("Add Scripting Node"));
	sceneEditorWindow.spawnUI(std::move(chooseAssetWindow));
}

void ScriptingGizmo::addNode(const String& type, Vector2f pos)
{
	auto& nodes = scriptGraph->getNodes();
	const uint32_t id = static_cast<uint32_t>(nodes.size());
	nodes.emplace_back(type, pos);
	saveEntityData();
	
	openNodeUI(id, {}, false);
}

ScriptingNodeEditor::ScriptingNodeEditor(ScriptingGizmo& gizmo, UIFactory& factory, const IEntityEditorFactory& entityEditorFactory, uint32_t nodeId, const IScriptNodeType& nodeType, std::optional<Vector2f> pos)
	: UIWidget("scripting_node_editor", {}, UISizer())
	, gizmo(gizmo)
	, factory(factory)
	, entityEditorFactory(entityEditorFactory)
	, nodeId(nodeId)
	, nodeType(nodeType)
	, curSettings(gizmo.getNode(nodeId).getSettings())
{
	if (pos) {
		setAnchor(UIAnchor(Vector2f(), Vector2f(0.0f, 0.5f), pos.value()));
	} else {
		setAnchor(UIAnchor());
	}
}

void ScriptingNodeEditor::onMakeUI()
{
	getWidgetAs<UILabel>("name")->setText(LocalisedString::fromUserString(nodeType.getName()));

	setHandle(UIEventType::ButtonClicked, "ok", [=] (const UIEvent& event)
	{
		applyChanges();
		destroy();
	});

	setHandle(UIEventType::ButtonClicked, "cancel", [=] (const UIEvent& event)
	{
		destroy();
	});

	setHandle(UIEventType::ButtonClicked, "delete", [=] (const UIEvent& event)
	{
		deleteNode();
		destroy();
	});

	setHandle(UIEventType::TextSubmit, [=] (const UIEvent& event)
	{
		applyChanges();
		destroy();
	});

	getWidget("delete")->setEnabled(nodeType.canDelete());

	makeFields(getWidget("nodeFields"));
}

void ScriptingNodeEditor::onAddedToRoot(UIRoot& root)
{
	factory.loadUI(*this, "ui/halley/scripting_node_editor");

	root.registerKeyPressListener(shared_from_this());
}

void ScriptingNodeEditor::onRemovedFromRoot(UIRoot& root)
{
	root.removeKeyPressListener(*this);
}

bool ScriptingNodeEditor::onKeyPress(KeyboardKeyPress key)
{
	if (key.is(KeyCode::Esc)) {
		destroy();
		return true;
	}

	if (key.is(KeyCode::Enter)) {
		applyChanges();
		destroy();
		return true;
	}

	return false;
}

void ScriptingNodeEditor::applyChanges()
{
	auto* gizmo = &this->gizmo;
	const auto nodeId = this->nodeId;
	auto curSettings = ConfigNode(this->curSettings);
	
	Concurrent::execute(gizmo->getExecutionQueue(), [gizmo, nodeId, curSettings = std::move(curSettings)] () {
		gizmo->getNode(nodeId).getSettings() = std::move(curSettings);
		gizmo->saveEntityData();
	});
}

void ScriptingNodeEditor::deleteNode()
{
	auto* gizmo = &this->gizmo;
	const auto nodeId = this->nodeId;

	Concurrent::execute(gizmo->getExecutionQueue(), [gizmo, nodeId] () {
		gizmo->destroyNode(nodeId);
	});
}

void ScriptingNodeEditor::makeFields(const std::shared_ptr<UIWidget>& fieldsRoot)
{
	fieldsRoot->clear();
	
	const auto& types = nodeType.getSettingTypes();

	for (const auto& type: types) {
		const auto params = ComponentFieldParameters(type.name, ComponentDataRetriever(curSettings, type.name), type.defaultValue);
		auto field = entityEditorFactory.makeField(type.type, params, ComponentEditorLabelCreation::Always);
		fieldsRoot->add(field);
	}

	bool foundFocus = false;
	fieldsRoot->descend([&] (const std::shared_ptr<UIWidget>& e)
	{
		if (!foundFocus && e->canReceiveFocus()) {
			foundFocus = true;
			getRoot()->setFocus(e);
		}
	}, false, true);

	if (!foundFocus) {
		getRoot()->setFocus(shared_from_this());
	}
}

ScriptingGizmoToolbar::ScriptingGizmoToolbar(UIFactory& factory, ScriptingGizmo& gizmo)
	: UIWidget("scripting_gizmo_toolbar", {}, UISizer())
	, gizmo(gizmo)
	, factory(factory)
{
	factory.loadUI(*this, "ui/halley/scripting_gizmo_toolbar");
	setInteractWithMouse(true);
}

void ScriptingGizmoToolbar::onMakeUI()
{
	setHandle(UIEventType::ButtonClicked, "addNode", [=] (const UIEvent& event)
	{
		gizmo.addNode();
	});
}

void ScriptingGizmoToolbar::onAddedToRoot(UIRoot& root)
{
	root.setFocus(shared_from_this());
	root.registerKeyPressListener(shared_from_this(), 1);
}

void ScriptingGizmoToolbar::onRemovedFromRoot(UIRoot& root)
{
	root.removeKeyPressListener(*this);
}

bool ScriptingGizmoToolbar::onKeyPress(KeyboardKeyPress key)
{
	if (key.is(KeyCode::A, KeyMods::Ctrl)) {
		gizmo.addNode();
		return true;
	}

	if (key.is(KeyCode::Delete)) {
		return gizmo.destroyHighlightedNode();
	}
	
	return false;
}
