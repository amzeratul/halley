#pragma once
#include "assets_editor_window.h"

namespace Halley {
	class PrefabEditor : public AssetEditor {
	public:
		PrefabEditor(UIFactory& factory, Resources& resources, AssetType type, Project& project, ProjectWindow& projectWindow);

		void reload() override;
		void onDoubleClick() override;
		
	private:
		ProjectWindow& projectWindow;

		void setupWindow();
	};
}
