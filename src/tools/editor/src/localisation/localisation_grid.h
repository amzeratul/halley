#pragma once

#include "localisation_data.h"
#include "halley/ui/widgets/ui_grid.h"

namespace Halley {
    class LocalisationGrid : public UIGrid {
    public:
        LocalisationGrid(UIFactory& factory);

    	void setData(const ILocOriginalData* origData, LocTranslationData* translatedData);

    protected:
        size_t getNumRows() const override;
        std::pair<Vector<float>, Vector<String>> getColumns() const override;
        void getLineDrawData(int idx, Vector<String>& strs, Vector<Colour4f>& colours) const override;
        const String& getKeyAt(int idx) const override;

    private:
        const ILocOriginalData* origData = nullptr;
        LocTranslationData* translatedData = nullptr;
        Colour4f outdatedCol;
    };
}
