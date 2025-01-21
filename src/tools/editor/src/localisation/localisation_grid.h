#pragma once

#include "localisation_data.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
    class LocalisationGrid : public UIWidget {
    public:
        LocalisationGrid(UIFactory& factory);

        void update(Time t, bool moved) override;
		void draw(UIPainter& painter) const override;

    	void setData(const ILocOriginalData* origData, LocTranslationData* translatedData);

    	int getSelectedLine() const;
        void setSelectedLine(int line);
        const String& getSelectedKey() const;

    protected:
        void onMouseOver(Vector2f mousePos) override;
        void onMouseLeft(Vector2f mousePos) override;
        void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;
        void releaseMouse(Vector2f mousePos, int button) override;

    private:
        UIFactory& factory;
        const ILocOriginalData* origData = nullptr;
        LocTranslationData* translatedData = nullptr;

        TextRenderer text;
        Colour4f textCol;
        Colour4f outdatedCol;

        std::optional<int> lineUnderMouse;
        std::optional<int> selectedLine;

		void drawLine(UIPainter& painter, int idx, const Vector<float>& columns) const;
		void drawLine(UIPainter& painter, Vector2f pos, gsl::span<const float> columns, gsl::span<const String> strings, gsl::span<const Colour4f> colours) const;
    };
}
