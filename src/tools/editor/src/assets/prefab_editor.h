#pragma once
#include "assets_editor_window.h"

namespace Halley {
	class PrefabEditor : public AssetEditor {
	public:
		PrefabEditor(UIFactory& factory, Resources& resources, AssetType type, Project& project, EditorRootStage& stage);

		void reload() override;

	private:
		EditorRootStage& stage;

		void setupWindow();
	};
}
