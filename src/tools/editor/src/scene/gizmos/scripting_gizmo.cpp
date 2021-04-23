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
}

void ScriptingGizmo::update(Time time, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState)
{
	if (!renderer) {
		renderer = std::make_shared<ScriptRenderer>(sceneEditor.getResources(), sceneEditor.getWorld(), *scriptNodeTypes, sceneEditorWindow.getProjectDefaultZoom());
	}
	renderer->setGraph(scriptGraph);

	if (!dragging) {
		nodeUnderMouse = renderer->getNodeIdxUnderMouse(basePos, getZoom(), inputState.mousePos);
	}

	if (!dragging && nodeUnderMouse && inputState.leftClickPressed && inputState.mousePos) {
		dragging = true;
		const auto nodePos = scriptGraph->getNodes()[nodeUnderMouse.value()].getPosition();
		startDragPos = nodePos - inputState.mousePos.value();
	}

	if (dragging) {
		if (inputState.mousePos) {
			const auto newPos = startDragPos + inputState.mousePos.value();
			scriptGraph->getNodes()[nodeUnderMouse.value()].setPosition(newPos);
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

	renderer->setHighlight(nodeUnderMouse);
	renderer->draw(painter, basePos, getZoom());
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
