#pragma once

#include "halley/ui/ui_widget.h"
#include <optional>
#include <halley/editor_extensions/choose_asset_window.h>

namespace Halley {
	class ProjectWindow;
	class EditorUIFactory;
	class UIFactory;
	class UIList;

    class AddComponentWindow : public ChooseAssetWindow {
    public:
        AddComponentWindow(UIFactory& factory, const std::vector<String>& componentList, Callback callback);
    };

	class ChooseAssetTypeWindow : public ChooseAssetWindow {
	public:
        ChooseAssetTypeWindow(UIFactory& factory, AssetType type, String defaultOption, Resources& gameResources, ProjectWindow& projectWindow, bool hasPreview, Callback callback);

    protected:
        std::shared_ptr<UIImage> makeIcon(const String& id, bool hasSearch) override;
		LocalisedString getItemLabel(const String& id, const String& name, bool hasSearch) override;
		std::shared_ptr<UISizer> makeItemSizer(std::shared_ptr<UIImage> icon, std::shared_ptr<UILabel> label, bool hasSearch) override;
		void sortItems(std::vector<std::pair<String, String>>& items) override;

		LocalisedString getPreviewItemLabel(const String& id, const String& name, bool hasSearch);
        std::shared_ptr<UIImage> makePreviewIcon(const String& id, bool hasSearch);
		std::shared_ptr<UISizer> makePreviewItemSizer(std::shared_ptr<UIImage> icon, std::shared_ptr<UILabel> label, bool hasSearch);

		ProjectWindow& projectWindow;
		AssetType type;

	private:
		Sprite icon;
		Sprite emptyPreviewIcon;
		Sprite emptyPreviewIconSmall;
		bool hasPreview;
	};

	class ChooseImportAssetWindow : public ChooseAssetWindow {
	public:
		ChooseImportAssetWindow(UIFactory& factory, Project& project, Callback callback);

	protected:
		std::shared_ptr<UIImage> makeIcon(const String& id, bool hasSearch) override;
		bool canShowAll() const override;

	private:
		Project& project;
		std::map<ImportAssetType, Sprite> icons;
	};

	class ChoosePrefabWindow : public ChooseAssetTypeWindow {
	public:
		ChoosePrefabWindow(UIFactory& factory, String defaultOption, Resources& gameResources, ProjectWindow& projectWindow, Callback callback);
	
    protected:
		void onCategorySet(const String& id) override;
	
	private:
		constexpr const static char* lastCategoryKey = "prefab_picker.last_category";
	};
}
