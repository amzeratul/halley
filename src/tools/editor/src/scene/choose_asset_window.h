#pragma once

#include "halley/ui/ui_widget.h"
#include <optional>

namespace Halley {
	class EditorUIFactory;
	class UIFactory;
	class UIList;

	class ChooseAssetWindow : public UIWidget {
    public:
        using Callback = std::function<void(std::optional<String>)>;
		
        ChooseAssetWindow(UIFactory& factory, Callback callback, bool canShowBlank = true);
		virtual ~ChooseAssetWindow();

        void onAddedToRoot(UIRoot& root) override;
		void setAssetIds(std::vector<String> ids, String defaultOption);
		void setAssetIds(std::vector<String> _ids, std::vector<String> _names, String _defaultOption);

		void setTitle(LocalisedString title);

    protected:
        bool onKeyPress(KeyboardKeyPress key) override;
		virtual bool canShowAll() const;
		virtual Sprite makeIcon(const String& id);
		EditorUIFactory& getFactory() const;

    private:
        EditorUIFactory& factory;
        Callback callback;
        std::shared_ptr<UIList> options;

		std::vector<String> ids;
		std::vector<String> names;
		std::vector<String>* effectiveNames;
		FuzzyTextMatcher fuzzyMatcher;
		String filter;
        String defaultOption;
        Colour4f highlightCol;
		bool canShowBlank = true;

        void makeUI();

        void accept();
        void cancel();
        void setFilter(const String& str);
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
        Sprite makeIcon(const String& id) override;

	private:
		AssetType type;
		Sprite icon;
	};

	class ChooseImportAssetWindow : public ChooseAssetWindow {
	public:
		ChooseImportAssetWindow(UIFactory& factory, Project& project, Callback callback);

	protected:
		Sprite makeIcon(const String& id) override;
		bool canShowAll() const override;

	private:
		Project& project;
		std::map<ImportAssetType, Sprite> icons;
	};
}
