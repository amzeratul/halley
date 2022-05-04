#pragma once

#include "halley/ui/ui_widget.h"

namespace Halley {
    class CurveEditor : public UIWidget {
    public:
        using Callback = std::function<void(const Vector<Vector2f>&)>;

        CurveEditor(String id, UIStyle style);

        void update(Time t, bool moved) override;
        void draw(UIPainter& painter) const override;

        void setHorizontalRange(Range<float> range);
        Range<float> getHorizontalRange() const;

        void setPoints(Vector<Vector2f> pts);
        const Vector<Vector2f>& getPoints() const;
        Vector<Vector2f>& getPoints();

        void setChangeCallback(Callback callback);

    protected:
        void onMouseOver(Vector2f mousePos) override;
        void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;
        void releaseMouse(Vector2f mousePos, int button) override;
        bool isFocusLocked() const override;
        bool canReceiveFocus() const override;

    private:
    	Sprite background;
        Sprite display;
        Colour4f lineColour;

    	Range<float> horizontalRange;
        Vector<Vector2f> points;
        Callback callback;

        std::optional<size_t> curAnchor;
        bool dragging = false;

        void normalizePoints();
        void notifyChange();
        Rect4f getDrawArea() const;
        void drawAnchor(Painter& painter, Vector2f pos, bool highlighted) const;

        Vector2f curveToMouseSpace(Vector2f curvePos) const;
        Vector2f mouseToCurveSpace(Vector2f mousePos) const;
        std::optional<size_t> getAnchorAt(Vector2f mousePos) const;
        Vector2f clampPoint(Vector2f point) const;

        void insertPoint(Vector2f curvePos);
        void deletePoint(size_t idx);
        void updateDragging(Vector2f mousePos);
    };
}
