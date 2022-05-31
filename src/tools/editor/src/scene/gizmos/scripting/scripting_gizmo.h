#pragma once
#include "scripting_base_gizmo.h"
#include "halley/editor_extensions/scene_editor_gizmo.h"

namespace Halley {
	class ScriptingGizmo final : public SceneEditorGizmo {
	public:
		ScriptingGizmo(SnapRules snapRules, UIFactory& factory, ISceneEditorWindow& sceneEditorWindow, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes);

		void update(Time time, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState) override;
		void draw(Painter& painter, const ISceneEditor& sceneEditor) const override;
		bool isHighlighted() const override;
		std::shared_ptr<UIWidget> makeUI() override;
		Vector<String> getHighlightedComponents() const override;
		void refreshEntity() override;
		void onEntityChanged() override;

	private:
		ScriptingBaseGizmo gizmo;
		ISceneEditorWindow& sceneEditorWindow;

		void loadEntityData();
		void saveEntityData();
		void compileEntityTargetList(World& world);
	};
}
