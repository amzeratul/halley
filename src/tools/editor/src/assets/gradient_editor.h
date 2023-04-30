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

    class GradientEditor : public UIWidget {
    public:
        using Callback = std::function<void(const ColourGradient&)>;

        GradientEditor(UIFactory& factory, String id, UIStyle style);

        void update(Time t, bool moved) override;
        void draw(UIPainter& painter) const override;

        void setGradient(ColourGradient gradient);
        const ColourGradient& getGradient() const;
        ColourGradient& getGradient();

        void setChangeCallback(Callback callback);

    protected:
        void onMouseOver(Vector2f mousePos) override;
        void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;
        void releaseMouse(Vector2f mousePos, int button) override;
        bool isFocusLocked() const override;
        bool canReceiveFocus() const override;

    private:
        UIFactory& factory;
        ColourGradient gradient;
        Callback callback;
    };
}
