#pragma once
#include "halley/editor_extensions/scene_editor_gizmo.h"

namespace Halley {
	class VariantGizmo final : public SceneEditorGizmo {
	public:
		explicit VariantGizmo(SnapRules snapRules, UIFactory& factory, ISceneEditorWindow& sceneEditorWindow);
		std::shared_ptr<UIWidget> makeUI() override;
		bool onKeyPress(KeyboardKeyPress key) override;

	private:
		UIFactory& factory;
		ISceneEditorWindow& sceneEditorWindow;
	};
}
