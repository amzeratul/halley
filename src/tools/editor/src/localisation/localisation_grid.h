#pragma once

#include "localisation_data.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
    class LocalisationGrid : public UIWidget {
    public:
        LocalisationGrid(UIFactory& factory);

        void update(Time t, bool moved) override;
		void draw(UIPainter& painter) const override;

    	void setData(const LocalisationDataChunk* origData, LocalisationDataChunk* curData);

    private:
        UIFactory& factory;
        const LocalisationDataChunk* origData = nullptr;
        LocalisationDataChunk* curData = nullptr;

        TextRenderer text;

		void drawLine(UIPainter& painter, int idx) const;
    };
}
