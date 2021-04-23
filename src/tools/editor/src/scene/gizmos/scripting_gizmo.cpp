#include "scripting_gizmo.h"
#include <components/script_component.h>
#include "halley/entity/components/transform_2d_component.h"

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

	if (!dragging && nodeUnderMouse && inputState.leftClickPressed && inputState.mousePos) {
		dragging = true;
		const auto nodePos = scriptGraph->getNodes()[nodeUnderMouse->first].getPosition();
		startDragPos = nodePos - inputState.mousePos.value();
	}

	if (dragging) {
		if (inputState.mousePos) {
			const auto newPos = startDragPos + inputState.mousePos.value();
			scriptGraph->getNodes()[nodeUnderMouse->first].setPosition(newPos);
		}
		if (!inputState.leftClickHeld) {
			dragging = false;
			saveEntityData();
		}
	}
}

void ScriptingGizmo::draw(Painter& painter) const
{
	if (!renderer) {
		return;
	}

	renderer->setHighlight(nodeUnderMouse ? nodeUnderMouse->first : std::optional<uint32_t>());
	renderer->draw(painter, basePos, getZoom());

	if (nodeUnderMouse && !dragging) {
		drawToolTip(painter, scriptGraph->getNodes().at(nodeUnderMouse->first), nodeUnderMouse->second);
	}
}

bool ScriptingGizmo::isHighlighted() const
{
	return !!nodeUnderMouse;
}

std::shared_ptr<UIWidget> ScriptingGizmo::makeUI()
{
	// TODO
	return {};
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

void ScriptingGizmo::drawToolTip(Painter& painter, const ScriptGraphNode& node, Rect4f nodePos) const
{
	const auto* nodeType = scriptNodeTypes->tryGetNodeType(node.getType());
	if (!nodeType) {
		return;
	}
	
	const auto [text, colours] = nodeType->getDescription(node);
	const float curZoom = getZoom();
	const auto pos = 0.5f * (nodePos.getBottomLeft() + nodePos.getBottomRight()) + Vector2f(0, 10) / curZoom;
	
	tooltipLabel
		.setText(text)
		.setColourOverride(colours)
		.setPosition(pos)
		.setAlignment(0.5f)
		.setSize(16 / curZoom)
		.setOutline(4.0f / curZoom)
		.draw(painter);
}
