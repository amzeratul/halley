#pragma once

#include "localisation_data.h"
#include "halley/ui/widgets/ui_grid.h"

namespace Halley {
    class LocalisationGrid : public UIGrid {
    public:
        LocalisationGrid(UIFactory& factory, const HalleyAPI& api, LocalisationFilterRules filterRules);

    	void setData(const ILocOriginalData* origData, LocTranslationData* translatedData, bool showProperties, I18NLanguage language);

        const String& getKeyAt(int idx) const override;

    	size_t getSrcRowCount() const override;

    protected:
        std::pair<Vector<float>, Vector<String>> getColumns() const override;
        void getLineDrawData(int idx, Vector<String>& strs, Vector<Colour4f>& colours, Vector<Sprite>& sprites) const override;
        void onRightClick(std::optional<int> line) override;
        void copySelection() override;
        LocalisedString getCellToolTip(int row, int col, const String& columnName) const override;

    private:
        UIFactory& factory;
        const HalleyAPI& api;
        LocalisationFilterRules filterRules;

        const ILocOriginalData* origData = nullptr;
        LocTranslationData* translatedData = nullptr;
        I18NLanguage language;
        Colour4f outdatedCol;

        HashMap<LocPriority, Sprite> priorityIcons;
        Sprite commentIcon;
        Sprite contextIcon;
        Sprite readyIcon;

        bool showProperties = false;

        void sendToClipboard(const String& str);

        bool isReadyToTranslate(const LocalisationDataEntry& entry) const;
    };
}
