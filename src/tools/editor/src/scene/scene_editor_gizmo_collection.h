#pragma once
#include <optional>
#include "halley/core/game/scene_editor_interface.h"
#include "halley/editor_extensions/scene_editor_gizmo.h"
#include "halley/time/halleytime.h"

namespace Halley {
	class Painter;
	class SceneEditorGizmo;

	class SceneEditorGizmoCollection : public ISceneEditorGizmoCollection {
	public:
		SceneEditorGizmoCollection(UIFactory& factory, Resources& resources, ISceneEditorWindow& sceneEditorWindow);
		
		bool update(Time time, const Camera& camera, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState, SceneEditorOutputState& outputState) override;
		void draw(Painter& painter) override;
		void setSelectedEntity(const std::optional<EntityRef>& entity, EntityData& entityData) override;
		void refreshEntity() override;
		void onEntityModified(const UUID& uuid, const EntityData& oldData, const EntityData& newData);
		std::shared_ptr<UIWidget> setTool(const String& tool, const String& componentName, const String& fieldName) override;
		void deselect() override;

		void addTool(const Tool& tool, GizmoFactory gizmoFactory) override;
		void resetTools();
		void clear();
		void generateList(UIList& list) override;
		gsl::span<const Tool> getTools() const { return tools; }

		ISceneEditorWindow& getSceneEditorWindow() override;

		bool onKeyPress(KeyboardKeyPress key, UIList& list);

	private:
		UIFactory& factory;
		Resources& resources;
		ISceneEditorWindow& sceneEditorWindow;
		SnapRules snapRules;

		std::vector<Tool> tools;
		std::map<String, GizmoFactory> gizmoFactories;
		
		std::unique_ptr<SceneEditorGizmo> selectedBoundsGizmo;
		std::unique_ptr<SceneEditorGizmo> selectionBoxGizmo;
		std::unique_ptr<SceneEditorGizmo> activeGizmo;
		
		String currentTool;
		
		std::optional<EntityRef> selectedEntity;
		EntityData* entityData = nullptr;
	};
}
