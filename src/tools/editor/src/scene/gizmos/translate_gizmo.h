#pragma once
#include "halley/editor_extensions/scene_editor_gizmo.h"

namespace Halley {
	enum class TranslateGizmoMode {
		Pivot,
		Centre
	};

	template <>
	struct EnumNames<TranslateGizmoMode> {
		constexpr std::array<const char*, 2> operator()() const {
			return{{
					"pivot",
					"centre"
				}};
		}
	};
	
	class TranslateGizmo final : public SceneEditorGizmo {
	public:
		explicit TranslateGizmo(SnapRules snapRules, UIFactory& factory, ISceneEditorWindow& sceneEditorWindow);
		void update(Time time, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState) override;
		void draw(Painter& painter) const override;
		bool isHighlighted() const override;
		std::shared_ptr<UIWidget> makeUI() override;
		std::vector<String> getHighlightedComponents() const override;
	
	private:
		UIFactory& factory;
		ISceneEditorWindow& sceneEditorWindow;
		bool visible = false;
		SceneEditorGizmoHandle handle;
		Vector2f handleOffset;
		TranslateGizmoMode mode;
		std::shared_ptr<UIList> uiMode;

		Circle getMainHandle() const;
		void updateEntityData(Vector2f pos);
		Vector2f getObjectOffset() const;
		void setMode(TranslateGizmoMode mode);
	};
}
