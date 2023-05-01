#pragma once

#include "halley/maths/colour_gradient.h"
#include "halley/ui/ui_widget.h"
#include "src/ui/popup_window.h"

namespace Halley {

    class GradientEditor;

    class GradientEditorButton : public UIImage {
    public:
        using Callback = std::function<void(ColourGradient)>;

    	GradientEditorButton(UIFactory& factory, VideoAPI& video, ColourGradient gradient, Callback callback);
        
    protected:
        void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;

    private:
        UIFactory& factory;
        VideoAPI& video;
        Callback callback;
        ColourGradient gradient;

        std::shared_ptr<Image> image;
        Sprite gradientImage;

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
        std::shared_ptr<GradientEditor> gradientEditor;

        void accept();
        void cancel();
    };

    class GradientEditor : public UIWidget {
    public:
        using Callback = std::function<void(const ColourGradient&)>;

        GradientEditor(UIFactory& factory, String id, UIStyle style, VideoAPI& videoAPI);

        void update(Time t, bool moved) override;
        void draw(UIPainter& painter) const override;

        void setGradient(ColourGradient gradient);
        const ColourGradient& getGradient() const;
        ColourGradient& getGradient();

        void setChangeCallback(Callback callback);

    protected:
        void onMouseOver(Vector2f mousePos) override;
        void onMouseLeft(Vector2f mousePos) override;
        void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;
        void releaseMouse(Vector2f mousePos, int button) override;
        bool isFocusLocked() const override;
        bool canReceiveFocus() const override;

    private:
        UIFactory& factory;
        ColourGradient gradient;
        Callback callback;
        VideoAPI& video;

    	Sprite anchorSprite;
        Sprite anchorColourSprite;

        std::optional<size_t> currentAnchor;
        std::optional<size_t> holdingAnchor;

        std::shared_ptr<Image> image;
        Sprite gradientImage;

        Rect4f getGradientBox() const;
        std::optional<size_t> getAnchorUnderMouse(Vector2f mousePos) const;

        void createAnchor(Vector2f mousePos);
        void insertAnchorAt(float pos, size_t idx);
        void editAnchor(size_t idx);
        void dragAnchor(size_t idx, Vector2f mousePos);

        void updateGradient();
    };
}
