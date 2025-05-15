#pragma once

#include "localisation_data.h"
#include "halley/ui/widgets/ui_grid.h"

namespace Halley {
    class LocalisationGrid : public UIGrid {
    public:
        LocalisationGrid(UIFactory& factory);

    	void setData(const ILocOriginalData* origData, LocTranslationData* translatedData, bool showProperties);

        LocalisedString getToolTip() const override;
        bool hasDynamicToolTip() const override;
        Vector2f getToolTipPosition(Vector2f mousePos) const override;
        const String& getKeyAt(int idx) const override;

    protected:
        size_t getNumRows() const override;
        std::pair<Vector<float>, Vector<String>> getColumns() const override;
        void getLineDrawData(int idx, Vector<String>& strs, Vector<Colour4f>& colours, Vector<Sprite>& sprites) const override;

    private:
        const ILocOriginalData* origData = nullptr;
        LocTranslationData* translatedData = nullptr;
        Colour4f outdatedCol;

        HashMap<LocPriority, Sprite> priorityIcons;
        Sprite commentIcon;
        Sprite contextIcon;

        bool showProperties = false;
    };
}
