#pragma once
#include <optional>
#include "halley/core/game/scene_editor_interface.h"
#include "halley/time/halleytime.h"
#include "scene_editor_gizmo.h"
#include "entity.h"

namespace Halley {
	class Painter;

	class SceneEditorGizmoCollection {
	public:
		SceneEditorGizmoCollection(Resources& resources);
		
		void update(Time time, const Camera& camera);
		void draw(Painter& painter);
		void setSelectedEntity(const std::optional<EntityRef>& entity);
		void setTool(SceneEditorTool tool);

	private:
		Resources& resources;
		std::unique_ptr<SceneEditorGizmo> selectedBoundsGizmo;
		std::unique_ptr<SceneEditorGizmo> activeGizmo;
		
		SceneEditorTool currentTool = SceneEditorTool::None;
		std::optional<EntityRef> selectedEntity;
	};
}
