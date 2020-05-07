#pragma once
#include <optional>
#include "halley/core/scene_editor/scene_editor_interface.h"
#include "halley/time/halleytime.h"
#include "scene_editor_gizmo.h"

namespace Halley {
	class Painter;

	class SceneEditorGizmoCollection : public ISceneEditorGizmoCollection {
	public:
		SceneEditorGizmoCollection(UIFactory& factory, Resources& resources);
		
		void update(Time time, const Camera& camera, const SceneEditorInputState& inputState, SceneEditorOutputState& outputState) override;
		void draw(Painter& painter) override;
		void setSelectedEntity(const std::optional<EntityRef>& entity, ConfigNode& entityData) override;
		std::shared_ptr<UIWidget> setTool(SceneEditorTool tool, const String& componentName, const String& fieldName, const ConfigNode& options) override;

	private:
		UIFactory& factory;
		Resources& resources;
		std::unique_ptr<SceneEditorGizmo> selectedBoundsGizmo;
		std::unique_ptr<SceneEditorGizmo> activeGizmo;
		
		SceneEditorTool currentTool = SceneEditorTool::None;
		
		std::optional<EntityRef> selectedEntity;
		ConfigNode* entityData = nullptr;
	};
}
