#include "scripting_gizmo.h"

#include <components/script_component.h>
#include <components/script_graph_component.h>
#include <components/script_target_component.h>

#include "halley/entity/components/transform_2d_component.h"

using namespace Halley;

ScriptingGizmo::ScriptingGizmo(SnapRules snapRules, UIFactory& factory, ISceneEditorWindow& sceneEditorWindow, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes)
	: SceneEditorGizmo(snapRules)
	, gizmo(factory, sceneEditorWindow.getEntityEditorFactory(), &sceneEditorWindow.getEntityFactory()->getWorld(), scriptNodeTypes, sceneEditorWindow.getProjectDefaultZoom())
	, sceneEditorWindow(sceneEditorWindow)
{
	gizmo.setModifiedCallback([=] ()
	{
		modified = true;
	});
	gizmo.setUIRoot(sceneEditorWindow.getUIRoot());
	compileEntityTargetList(sceneEditorWindow.getEntityFactory()->getWorld());
}

void ScriptingGizmo::update(Time time, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState)
{
	gizmo.setZoom(getZoom());
	gizmo.update(time, sceneEditor.getResources(), inputState);

	if (modified) {
		modified = false;
		saveEntityData();
	}
}

void ScriptingGizmo::draw(Painter& painter, const ISceneEditor& sceneEditor) const
{
	gizmo.draw(painter);
}

bool ScriptingGizmo::isHighlighted() const
{
	return gizmo.isHighlighted();
}

std::shared_ptr<UIWidget> ScriptingGizmo::makeUI()
{
	return gizmo.makeUI();
}

Vector<String> ScriptingGizmo::getHighlightedComponents() const
{
	return { "CometScript" };
}

void ScriptingGizmo::refreshEntity()
{
	loadEntityData();
}

void ScriptingGizmo::onEntityChanged()
{
	loadEntityData();
}

bool ScriptingGizmo::allowEntitySpriteSelection() const
{
	return false;
}

void ScriptingGizmo::loadEntityData()
{
	const auto* transform = getComponent<Transform2DComponent>();
	gizmo.setBasePosition(transform ? transform->getGlobalPosition() : Vector2f());

	auto* scriptGraph = getComponent<ScriptGraphComponent>();
	gizmo.setGraph(scriptGraph ? &scriptGraph->scriptGraph : nullptr);
}

void ScriptingGizmo::saveEntityData()
{
	ConfigNode scriptGraphData;
	if (gizmo.getGraphPtr()) {
		const auto context = sceneEditorWindow.getEntityFactory()->makeStandaloneContext();
		scriptGraphData = gizmo.getGraph().toConfigNode(context->getEntitySerializationContext());
	}
	
	auto* data = getComponentData("CometScript");
	if (data) {
		(*data)["script"] = scriptGraphData;
	}
	markModified("CometScript", "script");
}

void ScriptingGizmo::compileEntityTargetList(World& world)
{
	Vector<ScriptingBaseGizmo::EntityTarget> entityTargets;
	for (const auto& e: world.getEntities()) {
		if (e.hasComponent<ScriptTargetComponent>()) {
			const auto pos = e.getComponent<Transform2DComponent>().getGlobalPosition();
			entityTargets.emplace_back(ScriptingBaseGizmo::EntityTarget{ pos, e.getEntityId() });
		}
	}
	gizmo.setEntityTargets(std::move(entityTargets));
}
