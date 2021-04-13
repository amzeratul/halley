#pragma once
#include "halley/editor_extensions/scene_editor_gizmo.h"

namespace Halley {
	class ScriptingGizmo final : public SceneEditorGizmo {
	public:
		explicit ScriptingGizmo(SnapRules snapRules, UIFactory& factory, ISceneEditorWindow& sceneEditorWindow);
		void update(Time time, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState) override;
		void draw(Painter& painter) const override;
		bool isHighlighted() const override;
		std::shared_ptr<UIWidget> makeUI() override;
		std::vector<String> getHighlightedComponents() const override;
	
	private:
		UIFactory& factory;
		ISceneEditorWindow& sceneEditorWindow;
	};
}
