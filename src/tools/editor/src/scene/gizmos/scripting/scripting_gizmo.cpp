#include "scripting_gizmo.h"

#include <components/script_component.h>
#include <components/script_target_component.h>

#include "halley/entity/components/transform_2d_component.h"

using namespace Halley;

ScriptingGizmo::ScriptingGizmo(SnapRules snapRules, UIFactory& factory, ISceneEditorWindow& sceneEditorWindow, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes)
	: SceneEditorGizmo(snapRules)
	, gizmo(factory, sceneEditorWindow, scriptNodeTypes, sceneEditorWindow.getProjectDefaultZoom())
	, sceneEditorWindow(sceneEditorWindow)
{
	gizmo.setModifiedCallback([=] ()
	{
		saveEntityData();
	});
	gizmo.setUIRoot(sceneEditorWindow.getUIRoot());
	compileEntityTargetList(sceneEditorWindow.getEntityFactory()->getWorld());
}

void ScriptingGizmo::update(Time time, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState)
{
	gizmo.setZoom(getZoom());
	gizmo.update(time, sceneEditor.getResources(), inputState);
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
	gizmo.setBasePosition(transform ? transform->getGlobalPosition() : Vector2f());

	auto* script = getComponent<ScriptComponent>();
	gizmo.setGraph(script ? &script->scriptGraph : nullptr);
}

void ScriptingGizmo::saveEntityData()
{
	ConfigNode scriptGraphData;
	if (gizmo.getGraphPtr()) {
		const auto context = sceneEditorWindow.getEntityFactory()->makeStandaloneContext();
		scriptGraphData = gizmo.getGraph().toConfigNode(context->getEntitySerializationContext());
	}
	
	auto* data = getComponentData("Script");
	if (data) {
		(*data)["scriptGraph"] = scriptGraphData;
	}
	markModified("Script", "scriptGraph");
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
