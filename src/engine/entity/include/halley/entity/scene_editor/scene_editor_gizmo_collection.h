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
		
		void update(Time time, const Camera& camera, const SceneEditorInputState& inputState, SceneEditorOutputState& outputState);
		void draw(Painter& painter);
		void setSelectedEntity(const std::optional<EntityRef>& entity, ConfigNode& entityData);
		void setTool(SceneEditorTool tool, const String& componentName, const String& fieldName, const ConfigNode& options);

	private:
		Resources& resources;
		std::unique_ptr<SceneEditorGizmo> selectedBoundsGizmo;
		std::unique_ptr<SceneEditorGizmo> activeGizmo;
		
		SceneEditorTool currentTool = SceneEditorTool::None;
		std::optional<EntityRef> selectedEntity;
		ConfigNode* entityData = nullptr;
	};
}
