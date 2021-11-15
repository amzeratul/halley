#pragma once

#include "../asset_editor.h"

namespace Halley {
	class UIGraphNode;

	class UIEditor : public AssetEditor {
	public:
		UIEditor(UIFactory& factory, Resources& gameResources, Project& project, const HalleyAPI& api);

		void reload() override;
		void onMakeUI() override;

	protected:
		std::shared_ptr<const Resource> loadResource(const String& assetId) override;
		
		std::shared_ptr<const UIDefinition> uiDefinition;
		std::shared_ptr<UIWidget> display;
		std::unique_ptr<UIFactory> gameFactory;
		std::unique_ptr<I18N> gameI18N;

		void doLoadUI();
	};
}
