#pragma once

#include "../ui_widget.h"

namespace Halley {
    class UIFactory;
    
    class UIGrid : public UIWidget {
    public:
        using LineColourCallback = std::function<std::optional<Colour4f>(int)>;

        UIGrid(String id, UIFactory& factory);

        void update(Time t, bool moved) override;
		void draw(UIPainter& painter) const override;

        void setLineColourFilter(LineColourCallback callback);

    	int getActiveSelectedLine() const;
        const String& getActiveSelectedKey() const;
        void setSelectedLine(int line);
        const HashSet<int>& getSelectedLines() const;
        virtual const String& getKeyAt(int idx) const;

    protected:
        void onMouseOver(Vector2f mousePos) override;
        void onMouseLeft(Vector2f mousePos) override;
        void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;
        void releaseMouse(Vector2f mousePos, int button) override;
        bool isFocusLocked() const override;
        bool canReceiveFocus() const override;

        virtual size_t getNumRows() const;
        float getLineHeight() const;
        void onDataUpdated();

        virtual std::pair<Vector<float>, Vector<String>> getColumns() const;
        virtual void getLineDrawData(int idx, Vector<String>& strs, Vector<Colour4f>& colours, Vector<Sprite>& sprites) const;

        Vector2f getRowBasePos(int row) const;
        Vector2f getCellBasePos(int row, int column) const;

    	Colour4f textCol;

        std::optional<int> lineUnderMouse;
        std::optional<int> columnUnderMouse;

    	Vector<float> columns;
    	Vector<String> columnNames;

    private:
        UIFactory& factory;

        float fontSize = 13;
		float cellBorder = 3;

        LineColourCallback lineColourFilter;
        TextRenderer text;

        std::optional<int> boundedLineUnderMouse;
        std::optional<int> activeSelectedLine;
        HashSet<int> selectedLines;
        Vector<std::optional<Colour4f>> colours;

        std::optional<Vector2f> lastMousePos;

        std::optional<int> holdingLine;
        bool holdingMoved = false;

        Time scrollCooldown = 0;
        mutable std::optional<Rect4f> drawClip;

        void drawLine(UIPainter& painter, int idx, const Vector<float>& columns) const;
		void drawLine(UIPainter& painter, Vector2f pos, gsl::span<const float> columns, gsl::span<const String> strings, gsl::span<const Colour4f> colours, gsl::span<const Sprite> sprites) const;

        void onClickLine(std::optional<int> line, KeyMods mods);
    };
}
