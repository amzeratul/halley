#pragma once

#include "../asset_editor.h"

namespace Halley {
	class UIGraphNode;

	class UIEditor : public AssetEditor {
	public:
		UIEditor(UIFactory& factory, Resources& gameResources, Project& project);

		void reload() override;
		void onMakeUI() override;

	protected:
		std::shared_ptr<const Resource> loadResource(const String& assetId) override;
	};
}
