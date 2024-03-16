#include "halley/ui/ui_widget.h"
#include "halley/ui/widgets/ui_list.h"
#include "halley/ui/widgets/ui_paged_pane.h"
#include "src/scene/choose_window.h"

namespace Halley {
	class EditorUIFactory;
	class Project;
	class AssetEditor;
	class MetadataEditor;
	class ProjectWindow;
	
	class AssetEditorWindow : public UIWidget {
    public:
		explicit AssetEditorWindow(EditorUIFactory& factory, Project& project, ProjectWindow& projectWindow);

		void onMakeUI() override;
		void reload();

		void onDoubleClickAsset();
		void refreshAssets();

		void loadAsset(const String& name, std::optional<AssetType> type, bool force = false);

		Path getCurrentAssetPath() const;

		bool isModified() const;
		bool canSave(bool forceInstantCheck) const;
		void save();
		String getName() const;

		void onOpenAssetFinder(PaletteWindow& assetFinder);

	private:
		EditorUIFactory& factory;
		Project& project;
		ProjectWindow& projectWindow;
		std::shared_ptr<MetadataEditor> metadataEditor;
		
		String loadedAsset;
		std::optional<AssetType> loadedType;

		std::shared_ptr<UIList> contentList;
		std::shared_ptr<UIPagedPane> content;
		Vector<std::shared_ptr<AssetEditor>> curEditors;

		bool modified = false;
		Vector<std::pair<AssetType, String>> lastAssets;

		std::shared_ptr<AssetEditor> makeEditor(AssetType type);
		void createEditorTab(Path filePath, AssetType type, const String& name);
		Vector<std::pair<AssetType, String>> getAssetsFromFile(const Path& path) const;
	};
}
