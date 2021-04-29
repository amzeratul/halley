#include "scripting_gizmo.h"
#include <components/script_component.h>
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
}

void ScriptingGizmo::update(Time time, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState)
{
	if (!renderer) {
		renderer = std::make_shared<ScriptRenderer>(sceneEditor.getResources(), sceneEditor.getWorld(), *scriptNodeTypes, sceneEditorWindow.getProjectDefaultZoom());
	}
	renderer->setGraph(scriptGraph);

	if (!dragging) {
		nodeUnderMouse = renderer->getNodeUnderMouse(basePos, getZoom(), inputState.mousePos);
	}

	// This must be set before onNodeDragging/onEditingConnection below
	const bool startedIdle = !dragging && !nodeEditingConnection;

	if (dragging) {
		onNodeDragging(inputState);
	} else if (nodeEditingConnection) {
		onEditingConnection(inputState);
	}

	if (startedIdle && nodeUnderMouse && inputState.mousePos) {
		if (nodeUnderMouse->elementType == ScriptRenderer::NodeElementType::Node) {
			if (inputState.leftClickPressed) {
				onNodeClicked(inputState.mousePos.value());
			} else if (inputState.rightClickReleased) {
				openNodeUI(nodeUnderMouse->nodeId, inputState.rawMousePos.value());
			}
		} else {
			if (inputState.leftClickPressed) {
				onPinClicked();
			}
		}
	}
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

	const auto nodeId = nodeEditingConnection->nodeId;
	const auto pinId = nodeEditingConnection->elementId;
	auto& node = scriptGraph->getNodes().at(nodeId);
	
	if (nodeEditingConnection->elementType == ScriptRenderer::NodeElementType::Output) {
		// Erase connection coming out of this
		node.setOutput(pinId, {}, 0);
	}
	if (nodeEditingConnection->elementType == ScriptRenderer::NodeElementType::Input) {
		// Erase existing connection to this input
		for (auto& n: scriptGraph->getNodes()) {
			n.disconnectOutputsTo(nodeId, pinId);
		}
	}
	if (nodeEditingConnection->elementType == ScriptRenderer::NodeElementType::Target) {
		// Erase connection coming out of this
		node.setTarget(pinId, {});
	}
}

void ScriptingGizmo::onEditingConnection(const SceneEditorInputState& inputState)
{
	nodeConnectionDst = inputState.mousePos;

	if (nodeUnderMouse) {
		const auto srcType = nodeEditingConnection->elementType;
		const auto dstType = nodeUnderMouse->elementType;
		using ET = ScriptRenderer::NodeElementType;
		
		if ((srcType == ET::Input && dstType != ET::Output) || (srcType == ET::Output && dstType != ET::Input) || (srcType == ET::Target && dstType != ET::Target)) {
			nodeUnderMouse.reset();
		}
	}
	
	if (inputState.leftClickPressed) {
		if (nodeUnderMouse) {
			const auto nodeId = nodeEditingConnection->nodeId;
			const auto pinId = nodeEditingConnection->elementId;
			auto& node = scriptGraph->getNodes().at(nodeId);
			
			if (nodeEditingConnection->elementType == ScriptRenderer::NodeElementType::Output) {
				node.setOutput(pinId, nodeUnderMouse->nodeId, nodeUnderMouse->elementId);
			}
			if (nodeEditingConnection->elementType == ScriptRenderer::NodeElementType::Input) {
				auto& otherNode = scriptGraph->getNodes().at(nodeUnderMouse->nodeId);
				otherNode.setOutput(nodeUnderMouse->elementId, nodeId, pinId);
			}
			if (nodeEditingConnection->elementType == ScriptRenderer::NodeElementType::Target) {
				// TODO
			}
		}
		
		nodeEditingConnection.reset();
	}
	if (inputState.rightClickPressed) {
		nodeEditingConnection.reset();
	}
}

void ScriptingGizmo::draw(Painter& painter) const
{
	if (!renderer) {
		return;
	}

	const bool overNodeBody = nodeUnderMouse && nodeUnderMouse->elementType == ScriptRenderer::NodeElementType::Node;

	std::optional<ScriptRenderer::ConnectionPath> path;
	if (nodeEditingConnection && nodeConnectionDst) {
		path = ScriptRenderer::ConnectionPath{ nodeEditingConnection->pinPos, nodeConnectionDst.value(), nodeEditingConnection->elementType };
	}
	
	renderer->setHighlight(nodeUnderMouse);
	renderer->setCurrentPath(path);
	renderer->draw(painter, basePos, getZoom());

	if (overNodeBody && !dragging) {
		drawToolTip(painter, scriptGraph->getNodes().at(nodeUnderMouse->nodeId), nodeUnderMouse->nodeArea);
	}
}

bool ScriptingGizmo::isHighlighted() const
{
	return !!nodeUnderMouse || nodeEditingConnection;
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

void ScriptingGizmo::destroyNode(uint32_t id)
{
	auto& nodes = scriptGraph->getNodes();

	if (id < nodes.size()) {
		for (auto& n: nodes) {
			n.onNodeRemoved(id);
		}
		
		nodes.erase(nodes.begin() + id);
		saveEntityData();
	}
}

ScriptGraphNode& ScriptingGizmo::getNode(uint32_t id)
{
	return scriptGraph->getNodes().at(id);
}

void ScriptingGizmo::drawToolTip(Painter& painter, const ScriptGraphNode& node, Rect4f nodePos) const
{
	const auto* nodeType = scriptNodeTypes->tryGetNodeType(node.getType());
	if (!nodeType) {
		return;
	}
	
	const auto [text, colours] = nodeType->getDescription(node, sceneEditorWindow.getEntityFactory()->getWorld());
	const float curZoom = getZoom();
	const auto pos = 0.5f * (nodePos.getBottomLeft() + nodePos.getBottomRight()) + Vector2f(0, 10) / curZoom;
	
	tooltipLabel
		.setColourOverride(colours)
		.setPosition(pos)
		.setAlignment(0.5f)
		.setSize(16 / curZoom)
		.setOutline(4.0f / curZoom);

	tooltipLabel
		.setText(tooltipLabel.split(text, 250.0f / curZoom));

	const auto extents = tooltipLabel.getExtents();
	const Rect4f tooltipArea = Rect4f(pos + extents * Vector2f(-0.5f, 0), pos + extents * Vector2f(0.5f, 1.0f)).grow(4 / curZoom, 2 / curZoom, 4 / curZoom, 4 / curZoom);
	const auto poly = Polygon({ tooltipArea.getTopLeft(), tooltipArea.getTopRight(), tooltipArea.getBottomRight(), tooltipArea.getBottomLeft() });
	painter.drawPolygon(poly, Colour4f(0, 0, 0, 0.6f));

	tooltipLabel
		.draw(painter);
}

std::shared_ptr<UIWidget> ScriptingGizmo::makeUI()
{
	auto ui = factory.makeUI("ui/halley/scripting_gizmo_toolbar");
	ui->setInteractWithMouse(true);

	ui->setHandle(UIEventType::ButtonClicked, "addNode", [=] (const UIEvent& event)
	{
		addNode();
	});
	
	return ui;
}

void ScriptingGizmo::openNodeUI(uint32_t nodeId, Vector2f pos)
{
	ScriptGraphNode& node = getNode(nodeId);
	const auto* nodeType = scriptNodeTypes->tryGetNodeType(node.getType());
	if (nodeType) {
		sceneEditorWindow.spawnUI(std::make_shared<ScriptingNodeEditor>(*this, factory, sceneEditorWindow.getEntityEditorFactory(), nodeId, *nodeType, pos));
	}
}

void ScriptingGizmo::addNode()
{
	auto chooseAssetWindow = std::make_shared<ChooseAssetWindow>(factory, [=] (std::optional<String> result)
	{
		if (result) {
			addNode(result.value());
		}
	}, false);
	chooseAssetWindow->setAssetIds(scriptNodeTypes->getTypes(), "");
	chooseAssetWindow->setTitle(LocalisedString::fromHardcodedString("Add Scripting Node"));
	sceneEditorWindow.spawnUI(std::move(chooseAssetWindow));
}

void ScriptingGizmo::addNode(const String& type)
{
	Vector2f pos; // TODO?
	scriptGraph->getNodes().push_back(ScriptGraphNode(type, pos));
}

ScriptingNodeEditor::ScriptingNodeEditor(ScriptingGizmo& gizmo, UIFactory& factory, const IEntityEditorFactory& entityEditorFactory, uint32_t nodeId, const IScriptNodeType& nodeType, Vector2f pos)
	: UIWidget("scripting_node_editor", {}, UISizer())
	, gizmo(gizmo)
	, factory(factory)
	, entityEditorFactory(entityEditorFactory)
	, nodeId(nodeId)
	, nodeType(nodeType)
	, curSettings(gizmo.getNode(nodeId).getSettings())
{
	setAnchor(UIAnchor(Vector2f(), Vector2f(0.0f, 0.5f), pos));
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
	gizmo.getNode(nodeId).getSettings() = curSettings;
	gizmo.saveEntityData();
}

void ScriptingNodeEditor::deleteNode()
{
	gizmo.destroyNode(nodeId);
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
