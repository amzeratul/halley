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
        AddComponentWindow(UIFactory& factory, const Vector<String>& componentList, Callback callback);
    };

	class ChooseAssetTypeWindow : public ChooseAssetWindow {
	public:
        ChooseAssetTypeWindow(Vector2f minSize, UIFactory& factory, AssetType type, String defaultOption, Resources& gameResources, ProjectWindow& projectWindow, bool hasPreview, std::optional<String> allowEmpty, Callback callback);

    protected:
        std::shared_ptr<UIImage> makeIcon(const String& id, bool hasSearch) override;
		LocalisedString getItemLabel(const String& id, const String& name, bool hasSearch) override;
		std::shared_ptr<UISizer> makeItemSizer(std::shared_ptr<UIImage> icon, std::shared_ptr<UILabel> label, bool hasSearch) override;
		void sortItems(Vector<std::pair<String, String>>& items) override;

		LocalisedString getPreviewItemLabel(const String& id, const String& name, bool hasSearch);
        std::shared_ptr<UIImage> makePreviewIcon(const String& id, bool hasSearch);
		std::shared_ptr<UISizer> makePreviewItemSizer(std::shared_ptr<UIImage> icon, std::shared_ptr<UILabel> label, bool hasSearch);

        int getNumColumns(Vector2f scrollPaneSize) const override;
		
		ProjectWindow& projectWindow;
		AssetType type;

	private:
		Sprite icon;
		Sprite emptyPreviewIcon;
		Sprite emptyPreviewIconSmall;
		bool hasPreview;
	};

	class ChooseUIStyleWindow : public ChooseAssetWindow {
	public:
        ChooseUIStyleWindow(Vector2f minSize, UIFactory& factory, String uiClass, String defaultOption, Resources& gameResources, Callback callback);

    protected:
		AssetType type;
	};

	class PaletteWindow final : public ChooseAssetWindow {
	public:
		using IconRetriever = std::function<Sprite(const String& prefix, const String& id)>;

		PaletteWindow(UIFactory& factory, Project& project, std::optional<String> initialQuery, Callback callback);
		void setIconRetriever(IconRetriever retriever);

	protected:
		std::shared_ptr<UIImage> makeIcon(const String& id, bool hasSearch) override;
		bool canShowAll() const override;

	private:
		Project& project;
		std::map<ImportAssetType, Sprite> icons;
		IconRetriever iconRetriever;
	};

	class ChoosePrefabWindow : public ChooseAssetTypeWindow {
	public:
		ChoosePrefabWindow(UIFactory& factory, std::optional<String> defaultOption, Resources& gameResources, ProjectWindow& projectWindow, Callback callback);
	
    protected:
		void onCategorySet(const String& id) override;
		void onOptionSelected(const String& id) override;
		bool onDestroyRequested() override;
	
	private:
		constexpr const static char* lastCategoryKey = "prefab_picker.last_category";
		constexpr const static char* lastOptionKey = "prefab_picker.last_option";
		String lastOption;
	};

    class ChooseEntityWindow : public ChooseAssetWindow {
    public:
        ChooseEntityWindow(UIFactory& factory, const Vector<IEntityEditorCallbacks::EntityInfo>& entityList, Callback callback);

    protected:
		int getNumColumns(Vector2f scrollPaneSize) const override;
		std::shared_ptr<UIImage> makeIcon(const String& id, bool hasSearch) override;
		std::shared_ptr<UISizer> makeItemSizer(std::shared_ptr<UIImage> icon, std::shared_ptr<UILabel> label, bool hasSearch) override;

    private:
		HashMap<String, Sprite> icons;
    };
}
