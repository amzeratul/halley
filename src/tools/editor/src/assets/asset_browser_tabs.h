#pragma once
#include "halley/ui/ui_widget.h"

namespace Halley {
	class EditorUIFactory;
	class ProjectWindow;
	class HalleyAPI;
	class Project;
	class AssetEditorWindow;

	class AssetBrowserTabs final : public UIWidget {
	public:
		AssetBrowserTabs(EditorUIFactory& factory, Project& project, ProjectWindow& projectWindow);

		void load(std::optional<AssetType> assetType, const String& name);
		void refreshAssets();
		void setAssetSrcMode(bool srcMode);

	protected:
		void update(Time t, bool moved) override;
		
	private:
		EditorUIFactory& factory;
		Project& project;
		ProjectWindow& projectWindow;
		
		std::shared_ptr<UIList> tabs;
		std::shared_ptr<UIPagedPane> pages;
		std::vector<std::shared_ptr<AssetEditorWindow>> windows;
		std::vector<String> toClose;

		bool srcMode = false;

		void makeUI();
		void closeTab(const String& key);
	};
}
