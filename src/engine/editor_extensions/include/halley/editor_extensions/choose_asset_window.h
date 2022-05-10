#pragma once

#include "halley/ui/ui_widget.h"
#include "asset_preview_generator.h"
#include "halley/text/fuzzy_text_matcher.h"
#include <optional>

namespace Halley {
	class UIImage;
	class UIFactory;
	class UIList;
	class UILabel;

    class ChooseAssetWindow : public UIWidget {
    public:		
        using Callback = std::function<void(std::optional<String>)>;
		
        ChooseAssetWindow(Vector2f minSize, UIFactory& factory, Callback callback, std::optional<String> canShowBlank = "[None]");
		virtual ~ChooseAssetWindow();

        void onAddedToRoot(UIRoot& root) override;
		void setAssetIds(Vector<String> ids, String defaultOption);
		void setAssetIds(Vector<String> ids, Vector<String> names, String defaultOption);
		void setAssetIds(Vector<String> ids, Vector<String> names, String prefix, Callback callback);

		void setTitle(LocalisedString title);
		void setCategoryFilters(Vector<AssetCategoryFilter> filters, const String& defaultOption);

    protected:
        std::shared_ptr<UIList> options;

    	bool onKeyPress(KeyboardKeyPress key) override;
		virtual bool canShowAll() const;
		UIFactory& getFactory() const;
        void onMakeUI() override;

		virtual std::shared_ptr<UIImage> makeIcon(const String& id, bool hasSearch);
		virtual LocalisedString getItemLabel(const String& id, const String& name, bool hasSearch);

		virtual std::shared_ptr<UISizer> makeItemSizer(std::shared_ptr<UIImage> icon, std::shared_ptr<UILabel> label, bool hasSearch);
		std::shared_ptr<UISizer> makeItemSizerBigIcon(std::shared_ptr<UIImage> icon, std::shared_ptr<UILabel> label);

        virtual void onCategorySet(const String& id);
		virtual void onOptionSelected(const String& id);

        virtual void sortItems(Vector<std::pair<String, String>>& items);
		void sortItemsByName(Vector<std::pair<String, String>>& items);
		void sortItemsById(Vector<std::pair<String, String>>& items);

        virtual int getNumColumns(Vector2f scrollPaneSize) const;

		bool isShowingDefaultDataSet() const;

    private:
		struct DataSet {
			Vector<String> origIds;
			Vector<String> origNames;
			String prefix;
			Callback callback;
		};

		Vector<String> ids;
		Vector<String> names;
        String defaultOption;

    	UIFactory& factory;
		UISizerType orientation;

		Vector<DataSet> entries;
		size_t curEntry = 0;

		Vector<AssetCategoryFilter> categoryFilters;
		
		FuzzyTextMatcher fuzzyMatcher;
		String rawFilter;
		String filter;
        Colour4f highlightCol;
		std::optional<String> canShowBlank;
		
        void accept();
        void cancel();
		void cancelAllExcept(std::optional<size_t> idx);
        void setUserFilter(const String& str);
		void setCategoryFilter(const String& filterId);
		void populateList();
		void addItem(const String& id, const String& name, gsl::span<const std::pair<uint16_t, uint16_t>> matchPositions = {});

		DataSet& getEntry(const String& prefix);
		void updateCurrentDataSet(const String& str);
    };
}
