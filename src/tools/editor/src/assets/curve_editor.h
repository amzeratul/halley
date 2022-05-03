#pragma once

#include "halley/ui/ui_widget.h"

namespace Halley {
    class CurveEditor : public UIWidget {
    public:
        CurveEditor(const String& id, UIStyle style);

        void update(Time t, bool moved) override;
        void draw(UIPainter& painter) const override;

        void setRange(Range<float> range);
        Range<float> getRange() const;

        void setPoints(Vector<Vector2f> points);
        const Vector<Vector2f>& getPoints() const;
        Vector<Vector2f>& getPoints();

    protected:
        void onMouseOver(Vector2f mousePos) override;
        void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;
        void releaseMouse(Vector2f mousePos, int button) override;

    private:
    	Sprite background;
        Range<float> range;
        Vector<Vector2f> points;
    };
}
