#pragma once

#include "localisation_data.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
    class LocalisationGrid : public UIWidget {
    public:
        using LineColourCallback = std::function<std::optional<Colour4f>(int)>;

        LocalisationGrid(UIFactory& factory);

        void update(Time t, bool moved) override;
		void draw(UIPainter& painter) const override;

    	void setData(const ILocOriginalData* origData, LocTranslationData* translatedData);
        void setLineColourFilter(LineColourCallback callback);

    	int getActiveSelectedLine() const;
        const String& getActiveSelectedKey() const;
        void setSelectedLine(int line);

    protected:
        void onMouseOver(Vector2f mousePos) override;
        void onMouseLeft(Vector2f mousePos) override;
        void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;
        void releaseMouse(Vector2f mousePos, int button) override;

    private:
        UIFactory& factory;
        const ILocOriginalData* origData = nullptr;
        LocTranslationData* translatedData = nullptr;

        LineColourCallback lineColourFilter;

        TextRenderer text;
        Colour4f textCol;
        Colour4f outdatedCol;

        std::optional<int> lineUnderMouse;
        std::optional<int> activeSelectedLine;
        HashSet<int> selectedLines;
        Vector<std::optional<Colour4f>> colours;

		void drawLine(UIPainter& painter, int idx, const Vector<float>& columns) const;
		void drawLine(UIPainter& painter, Vector2f pos, gsl::span<const float> columns, gsl::span<const String> strings, gsl::span<const Colour4f> colours) const;

        void onClickLine(std::optional<int> line, KeyMods mods);
    };
}
