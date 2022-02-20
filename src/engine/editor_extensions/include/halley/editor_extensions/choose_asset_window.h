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
		
        ChooseAssetWindow(UIFactory& factory, Callback callback, bool canShowBlank = true, UISizerType orientation = UISizerType::Vertical, int nColumns = 1);
		virtual ~ChooseAssetWindow();

        void onAddedToRoot(UIRoot& root) override;
		void setAssetIds(Vector<String> ids, String defaultOption);
		void setAssetIds(Vector<String> _ids, Vector<String> _names, String _defaultOption);

		void setTitle(LocalisedString title);
		void setCategoryFilters(Vector<AssetCategoryFilter> filters, const String& defaultOption);

    protected:
        bool onKeyPress(KeyboardKeyPress key) override;
		virtual bool canShowAll() const;
		UIFactory& getFactory() const;
        std::shared_ptr<UIList> options;
        void onMakeUI() override;

		virtual std::shared_ptr<UIImage> makeIcon(const String& id, bool hasSearch);
		virtual LocalisedString getItemLabel(const String& id, const String& name, bool hasSearch);

		virtual std::shared_ptr<UISizer> makeItemSizer(std::shared_ptr<UIImage> icon, std::shared_ptr<UILabel> label, bool hasSearch);
		std::shared_ptr<UISizer> makeItemSizerBigIcon(std::shared_ptr<UIImage> icon, std::shared_ptr<UILabel> label);

        virtual void onCategorySet(const String& id);

        virtual void sortItems(Vector<std::pair<String, String>>& items);
		void sortItemsByName(Vector<std::pair<String, String>>& items);
		void sortItemsById(Vector<std::pair<String, String>>& items);

    private:
        UIFactory& factory;
        Callback callback;
		UISizerType orientation;
		int nColumns;

		Vector<String> origIds;
		Vector<String> origNames;
		Vector<String> ids;
		Vector<String> names;
		Vector<AssetCategoryFilter> categoryFilters;
		
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
}
