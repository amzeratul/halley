#pragma once

#include "halley/maths/interpolation_curve.h"
#include "halley/ui/ui_widget.h"
#include "src/ui/popup_window.h"

namespace Halley {
    
    class CurveEditorButton : public UIImage {
    public:
        using Callback = std::function<void(InterpolationCurve)>;

    	CurveEditorButton(UIFactory& factory, InterpolationCurve curve, Callback callback);

        void update(Time t, bool moved) override;
		void draw(UIPainter& painter) const override;
        
    protected:
        void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;

    private:
        UIFactory& factory;
        Callback callback;
        InterpolationCurve curve;
        Colour4f lineColour;
        Vector<Vector2f> line;

        void updateLine();
    };

    class CurveEditorWindow : public PopupWindow {
    public:
        using Callback = std::function<void(InterpolationCurve)>;

        CurveEditorWindow(UIFactory& factory, InterpolationCurve curve, Callback callback);

        void onAddedToRoot(UIRoot& root) override;
        void onRemovedFromRoot(UIRoot& root) override;
        bool onKeyPress(KeyboardKeyPress key) override;

    protected:
        void onMakeUI() override;

    private:
        UIFactory& factory;
        Callback callback;
        InterpolationCurve curve;

        void accept();
        void cancel();
    };
}
