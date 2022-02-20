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
		void replaceAssetTab(const String& oldName, const String& newName);
		bool requestQuit(std::function<void()>);
		bool proceedQuitRequested(size_t idx, bool invoke);
		void saveCurrentTab();
		void saveAllTabs();
		void closeCurrentTab();
		void moveTabFocus(int delta);

	protected:
		void update(Time t, bool moved) override;
		
	private:
		EditorUIFactory& factory;
		Project& project;
		ProjectWindow& projectWindow;
		
		std::shared_ptr<UIList> tabs;
		std::shared_ptr<UIPagedPane> pages;
		Vector<std::shared_ptr<AssetEditorWindow>> windows;
		Vector<String> toClose;

		bool srcMode = false;
		bool waitingLoad = true;
		std::function<void()> quittingCallback;

		void makeUI();
		void openTab(std::optional<AssetType> assetType, const String& name, bool selected);
		bool closeTab(const String& key);
		void doCloseTab(const String& key);
		void populateTab(UIWidget& tab, std::optional<AssetType> assetType, const String& name, const String& key);

		void saveTabs();
		void loadTabs();

		void openContextMenu(const String& tabId);
		void onContextMenuAction(const String& action, const String& tabId);
	};
}
