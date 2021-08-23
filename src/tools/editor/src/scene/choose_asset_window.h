#pragma once

#include "halley/ui/ui_widget.h"
#include <optional>

namespace Halley {
	class SceneEditorWindow;
	class EditorUIFactory;
	class UIFactory;
	class UIList;

	class ChooseAssetWindow : public UIWidget {
    public:		
        using Callback = std::function<void(std::optional<String>)>;
		
        ChooseAssetWindow(UIFactory& factory, Callback callback, bool canShowBlank = true, UISizerType orientation = UISizerType::Vertical, int nColumns = 1);
		virtual ~ChooseAssetWindow();

        void onAddedToRoot(UIRoot& root) override;
		void setAssetIds(std::vector<String> ids, String defaultOption);
		void setAssetIds(std::vector<String> _ids, std::vector<String> _names, String _defaultOption);

		void setTitle(LocalisedString title);
		void setCategoryFilters(std::vector<AssetCategoryFilter> filters);

    protected:
        bool onKeyPress(KeyboardKeyPress key) override;
		virtual bool canShowAll() const;
		EditorUIFactory& getFactory() const;
        std::shared_ptr<UIList> options;
        void onMakeUI() override;

		virtual std::shared_ptr<UIImage> makeIcon(const String& id, bool hasSearch);
		virtual LocalisedString getItemLabel(const String& id, const String& name, bool hasSearch);

		virtual std::shared_ptr<UISizer> makeItemSizer(std::shared_ptr<UIImage> icon, std::shared_ptr<UILabel> label, bool hasSearch);
		std::shared_ptr<UISizer> makeItemSizerBigIcon(std::shared_ptr<UIImage> icon, std::shared_ptr<UILabel> label);

        virtual void sortItems(std::vector<std::pair<String, String>>& items);
		void sortItemsByName(std::vector<std::pair<String, String>>& items);
		void sortItemsById(std::vector<std::pair<String, String>>& items);

    private:
        EditorUIFactory& factory;
        Callback callback;
		UISizerType orientation;
		int nColumns;

		std::vector<String> origIds;
		std::vector<String> origNames;
		std::vector<String> ids;
		std::vector<String> names;
		std::vector<AssetCategoryFilter> categoryFilters;
		
		FuzzyTextMatcher fuzzyMatcher;
		String filter;
        String defaultOption;
        Colour4f highlightCol;
		bool canShowBlank = true;

        void accept();
        void cancel();
        void setUserFilter(const String& str);
		void setCategoryFilter(const String& filterId);
		void populateList();
		void addItem(const String& id, const String& name, gsl::span<const std::pair<uint16_t, uint16_t>> matchPositions = {});
    };

    class AddComponentWindow : public ChooseAssetWindow {
    public:
        AddComponentWindow(UIFactory& factory, const std::vector<String>& componentList, Callback callback);
    };

	class ChooseAssetTypeWindow : public ChooseAssetWindow {
	public:
        ChooseAssetTypeWindow(UIFactory& factory, AssetType type, String defaultOption, Resources& gameResources, Callback callback);

    protected:
        std::shared_ptr<UIImage> makeIcon(const String& id, bool hasSearch) override;

	private:
		AssetType type;
		Sprite icon;
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

	class ChoosePrefabWindow : public ChooseAssetWindow {
	public:
		ChoosePrefabWindow(UIFactory& factory, String defaultOption, Resources& gameResources, SceneEditorWindow& sceneEditorWindow, Callback callback);
	
    protected:
		LocalisedString getItemLabel(const String& id, const String& name, bool hasSearch) override;
        std::shared_ptr<UIImage> makeIcon(const String& id, bool hasSearch) override;
		std::shared_ptr<UISizer> makeItemSizer(std::shared_ptr<UIImage> icon, std::shared_ptr<UILabel> label, bool hasSearch) override;
		void sortItems(std::vector<std::pair<String, String>>& items) override;
	
	private:
		Sprite icon;
		Sprite emptyPreviewIcon;
		Sprite emptyPreviewIconSmall;
		SceneEditorWindow& sceneEditorWindow;
	};
}
