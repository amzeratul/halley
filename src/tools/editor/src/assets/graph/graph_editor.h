#pragma once

#include "../asset_editor.h"
#include "src/ui/scroll_background.h"

namespace Halley {
	class GraphEditor : public AssetEditor {
	public:
		GraphEditor(UIFactory& factory, Resources& gameResources, Project& project, AssetType type);

		void onMakeUI() override;

	protected:
		std::shared_ptr<ScrollBackground> scrollBg;

		void addNode(Vector2f pos);
	};
}
