#include "scene_editor/scene_editor_gizmo_collection.h"
#include "gizmos/translate_gizmo.h"
#include "gizmos/selected_bounds_gizmo.h"
#include "entity.h"
#include "halley/core/graphics/camera.h"
using namespace Halley;

SceneEditorGizmoCollection::SceneEditorGizmoCollection(Resources& resources)
	: resources(resources)
{
	selectedBoundsGizmo = std::make_unique<SelectedBoundsGizmo>(resources);
}

void SceneEditorGizmoCollection::update(Time time, const Camera& camera, const SceneEditorInputState& inputState)
{
	selectedBoundsGizmo->setCamera(camera);
	selectedBoundsGizmo->update(time, inputState);
	
	if (activeGizmo) {
		activeGizmo->setCamera(camera);
		activeGizmo->update(time, inputState);
	}
}

void SceneEditorGizmoCollection::draw(Painter& painter)
{
	selectedBoundsGizmo->draw(painter);
	
	if (activeGizmo) {
		activeGizmo->draw(painter);
	}
}

void SceneEditorGizmoCollection::setSelectedEntity(const std::optional<EntityRef>& entity)
{
	selectedEntity = entity;
	
	selectedBoundsGizmo->setSelectedEntity(entity);
	
	if (activeGizmo) {
		activeGizmo->setSelectedEntity(entity);
	}
}

void SceneEditorGizmoCollection::setTool(SceneEditorTool tool)
{
	if (tool != currentTool) {
		currentTool = tool;
		activeGizmo.reset();
		
		switch (tool) {
		case SceneEditorTool::Translate:
			activeGizmo = std::make_unique<TranslateGizmo>();
		}

		if (activeGizmo) {
			activeGizmo->setSelectedEntity(selectedEntity);
		}
	}
}
