#include "halley/ui/ui_widget.h"
#include "halley/ui/widgets/ui_list.h"
#include "halley/ui/widgets/ui_paged_pane.h"

namespace Halley {
	class Project;
	class AssetEditor;
	class MetadataEditor;
	class ProjectWindow;
	
	class AssetEditorWindow : public UIWidget {
    public:
		explicit AssetEditorWindow(UIFactory& factory);

		void onMakeUI() override;
		void init(Project& project, ProjectWindow& projectWindow, std::shared_ptr<MetadataEditor> metadataEditor);
		void setAssetSrcMode(bool assetSrcMode);
		void onDoubleClickAsset();
		void refreshAssets();

		void loadAsset(const String& name, std::optional<AssetType> type, bool clearDropdown, bool force = false);

		Path getCurrentAssetPath() const;

	private:
		UIFactory& factory;
		Project* project;
		ProjectWindow* projectWindow;
		std::shared_ptr<MetadataEditor> metadataEditor;
		bool assetSrcMode = false;
		
		String loadedAsset;
		std::optional<AssetType> loadedType;

		std::shared_ptr<UIDropdown> contentListDropdown;
		std::shared_ptr<UIList> contentList;
		std::shared_ptr<UIPagedPane> content;
		std::vector<std::shared_ptr<AssetEditor>> curEditors;

		std::shared_ptr<AssetEditor> makeEditor(Path filePath, AssetType type, const String& name);
		void createEditorTab(Path filePath, AssetType type, const String& name);
	};
}
