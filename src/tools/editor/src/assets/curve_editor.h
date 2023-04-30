#pragma once

#include "halley/maths/interpolation_curve.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
    class CurveEditor : public UIWidget {
    public:
        using Callback = std::function<void(const InterpolationCurve&)>;

        CurveEditor(UIFactory& factory, String id, UIStyle style);

        void update(Time t, bool moved) override;
        void draw(UIPainter& painter) const override;

        void setHorizontalRange(Range<float> range);
        Range<float> getHorizontalRange() const;
        void setHorizontalDividers(size_t n);
        void setVerticalDividers(size_t n);

        void setCurve(InterpolationCurve curve);
        const InterpolationCurve& getCurve() const;
        InterpolationCurve& getCurve();

        void setChangeCallback(Callback callback);

    protected:
        void onMouseOver(Vector2f mousePos) override;
        void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;
        void releaseMouse(Vector2f mousePos, int button) override;
        bool isFocusLocked() const override;
        bool canReceiveFocus() const override;

    private:
        UIFactory& factory;

    	Sprite background;
        Sprite display;
        Sprite gridLine;
        Colour4f lineColour;
        TextRenderer tooltipLabel;

    	Range<float> horizontalRange;
        size_t nHorizontalDividers = 10;
        size_t nVerticalDividers = 5;
        InterpolationCurve curve;
        Callback callback;

        std::optional<size_t> curAnchor;
        std::optional<size_t> curSegment;
        std::optional<Vector2f> mouseAnchor;
        bool dragging = false;

        void normalizePoints();
        void notifyChange();
        Rect4f getDrawArea() const;

        void drawLine(Painter& painter) const;
        void drawAnchor(Painter& painter, Vector2f pos, bool highlighted) const;

        Vector2f curveToMouseSpace(Vector2f curvePos) const;
        Vector2f mouseToCurveSpace(Vector2f mousePos) const;
        std::optional<size_t> getAnchorAt(Vector2f mousePos) const;
        std::optional<size_t> getSegmentAt(Vector2f mousePos) const;
        Vector2f clampPoint(Vector2f point) const;

        void insertPoint(Vector2f curvePos);
        void deletePoint(size_t idx);
        void updateDragging(Vector2f mousePos);
        void editSegment(size_t idx);
    };
}
