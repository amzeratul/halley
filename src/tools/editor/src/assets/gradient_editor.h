#pragma once

#include "halley/maths/colour_gradient.h"
#include "halley/ui/ui_widget.h"
#include "src/ui/popup_window.h"

namespace Halley {
    
    class GradientEditorButton : public UIImage {
    public:
        using Callback = std::function<void(ColourGradient)>;

    	GradientEditorButton(UIFactory& factory, ColourGradient gradient, Callback callback);
        
    protected:
        void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;

    private:
        UIFactory& factory;
        Callback callback;
        ColourGradient gradient;

        void updateGradient();
    };

    class GradientEditorWindow : public PopupWindow {
    public:
        using Callback = std::function<void(ColourGradient)>;

        GradientEditorWindow(UIFactory& factory, ColourGradient gradient, Callback callback);

        void onAddedToRoot(UIRoot& root) override;
        void onRemovedFromRoot(UIRoot& root) override;
        bool onKeyPress(KeyboardKeyPress key) override;

    protected:
        void onMakeUI() override;

    private:
        UIFactory& factory;
        Callback callback;
        ColourGradient gradient;

        void accept();
        void cancel();
    };
}
