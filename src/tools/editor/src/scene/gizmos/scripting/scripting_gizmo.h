#pragma once
#include "halley/editor_extensions/scene_editor_gizmo.h"

namespace Halley {
	class ScriptingGizmo final : public SceneEditorGizmo {
	public:
		ScriptingGizmo(SnapRules snapRules, UIFactory& factory, ISceneEditorWindow& sceneEditorWindow, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes);

		Vector<String> getHighlightedComponents() const override;
		bool allowEntitySpriteSelection() const override;

	private:
		ISceneEditorWindow& sceneEditorWindow;
		UIFactory& factory;
		std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes;
	};
}
