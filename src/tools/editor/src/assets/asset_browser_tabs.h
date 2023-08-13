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

		void load(const String& name);
		void refreshAssets();
		void replaceAssetTab(const String& oldName, const String& newName);
		bool requestQuit(std::function<void()>);
		bool proceedQuitRequested(size_t idx, bool invoke);
		void saveCurrentTab();
		void saveAllTabs();
		void closeCurrentTab();
		bool closeTab(const String& key);
		bool reloadTab(const String& key);
		void renameTab(const String& id, const String& newId, std::optional<AssetType> assetType);
		void moveTabFocus(int delta);
		std::shared_ptr<AssetEditorWindow> getActiveWindow() const;
		String getCurrentAssetId() const;

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
		Vector<String> toReload;

		std::function<void()> quittingCallback;

		void makeUI();
		void openTab(std::optional<AssetType> assetType, const String& name, bool selected);
		bool confirmTabAction(const String& key, std::function<void(bool)> action);
		void doCloseTab(const String& key);
		void doReloadTab(const String& key);
		void populateTab(UIWidget& tab, std::optional<AssetType> assetType, const String& name, const String& key);
		void updateTab(UIWidget& tab, const String& name, const String& key);

		void saveTabs();
		void loadTabs();

		void openContextMenu(const String& tabId);
		void onContextMenuAction(const String& action, const String& tabId);
	};
}
