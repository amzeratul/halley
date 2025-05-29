#pragma once

#include "localisation_data.h"
#include "halley/ui/widgets/ui_grid.h"

namespace Halley {
    class LocalisationGrid : public UIGrid {
    public:
        LocalisationGrid(UIFactory& factory, const HalleyAPI& api, LocalisationFilterRules filterRules);

    	void setData(const ILocOriginalData* origData, LocTranslationData* translatedData, bool showProperties);

        LocalisedString getToolTip() const override;
        bool hasDynamicToolTip() const override;
        Vector2f getToolTipPosition(Vector2f mousePos) const override;
        const String& getKeyAt(int idx) const override;

    	size_t getSrcRowCount() const override;

    protected:
        std::pair<Vector<float>, Vector<String>> getColumns() const override;
        void getLineDrawData(int idx, Vector<String>& strs, Vector<Colour4f>& colours, Vector<Sprite>& sprites) const override;
        void onRightClick(std::optional<int> line) override;
        void copySelection() override;

    private:
        UIFactory& factory;
        const HalleyAPI& api;
        LocalisationFilterRules filterRules;

        const ILocOriginalData* origData = nullptr;
        LocTranslationData* translatedData = nullptr;
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
