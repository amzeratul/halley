#pragma once

namespace Halley {
	class Project;

	class AssetsEditorWindow : public UIWidget {
    public:
        AssetsEditorWindow(UIFactory& factory, Project& project, const HalleyAPI& api);

	private:
		UIFactory& factory;
		Project& project;

		std::unique_ptr<Resources> gameResources;

		void makeUI();
		void loadResources(const HalleyAPI& api);
		void listAssets(AssetType type);
    };
}