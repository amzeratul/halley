#pragma once
#include "halley/editor_extensions/scene_editor_gizmo.h"

namespace Halley {
	class VariantGizmo final : public SceneEditorGizmo {
	public:
		explicit VariantGizmo(SnapRules snapRules, UIFactory& factory, ISceneEditorWindow& sceneEditorWindow);
		std::shared_ptr<UIWidget> makeUI() override;

	private:
		UIFactory& factory;
		ISceneEditorWindow& sceneEditorWindow;

		std::shared_ptr<UIWidget> ui;
		std::shared_ptr<UIList> variantsList;

		Vector<SceneVariant> variants;

		void loadVariants();
		void saveVariants();

		void populateVariants(int startIdx);
		void populateVariantInfo();
		void addVariant();
		void removeVariant();

		void setVariant(const String& variant);
		String getVariant() const;
	};
}
