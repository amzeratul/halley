#include "scene_editor/scene_editor_gizmo_collection.h"
#include "scene_editor/translate_gizmo.h"
#include "entity.h"
using namespace Halley;

void SceneEditorGizmoCollection::update(Time time)
{
	if (activeGizmo) {
		activeGizmo->update(time);
	}
}

void SceneEditorGizmoCollection::draw(const Painter& painter)
{
	if (activeGizmo) {
		activeGizmo->draw(painter);
	}
}

void SceneEditorGizmoCollection::setSelectedEntity(const std::optional<EntityRef>& entity)
{
	selectedEntity = entity;
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
