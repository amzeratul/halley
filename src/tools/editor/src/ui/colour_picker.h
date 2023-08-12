#pragma once

#include <halley/ui/halley_ui.h>

#include "popup_window.h"

namespace Halley {
	class ColourPickerDisplay;

	class ColourPickerButton : public UIImage {
    public:
        using Callback = std::function<void(String, bool)>;

    	ColourPickerButton(UIFactory& factory, String colour, Callback callback);

        void update(Time t, bool moved) override;
		void draw(UIPainter& painter) const override;
        
    	void setColour(String colour, bool final);

    protected:
        void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;

    private:
        UIFactory& factory;
        Callback callback;
        TextRenderer label;
        String colour;

        Colour4f readColour(const String& col);
	};

    class ColourPicker : public PopupWindow {
    public:
        using Callback = std::function<void(Colour4f, bool)>;

    	ColourPicker(UIFactory& factory, Colour4f initialColour, Callback callback);

        void onAddedToRoot(UIRoot& root) override;
        void onRemovedFromRoot(UIRoot& root) override;
        bool onKeyPress(KeyboardKeyPress key) override;

        Colour4f getColour() const;
        void setColour(Colour4f col);

        void update(Time t, bool moved) override;

    private:
        Colour4f initialColour;
        Colour4f colour;
        Callback callback;

        std::shared_ptr<ColourPickerDisplay> mainDisplay;
        std::shared_ptr<ColourPickerDisplay> ribbonDisplay;
        
        std::array<std::shared_ptr<UISlider>, 6> rgbhsvSliders;
        std::shared_ptr<UISlider> alphaSlider;
        std::shared_ptr<UIImage> colourView;
        std::shared_ptr<UIImage> prevColourView;
        std::shared_ptr<UITextInput> hexCode;
        std::shared_ptr<UITextInput> floatCode;

    	bool updatingUI = false;
        bool updatingHex = false;
        bool updatingDisplay = false;

        void onMakeUI() override;

        void accept();
        void cancel();
        void onColourChanged();

        void updateUI();
    };

    class ColourPickerDisplay : public UIImage {
    public:
        enum class CursorType {
	        Circle,
            HorizontalLine
        };

        using Callback = std::function<void(Vector2f)>;

        ColourPickerDisplay(String id, Vector2f size, Resources& resources, const String& material);

        void update(Time t, bool moved) override;
        void draw(UIPainter& painter) const override;

        void setCallback(Callback callback);

        void setValue(Vector2f value);
        Vector2f getValue() const;

        void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;
        void releaseMouse(Vector2f mousePos, int button) override;
        void onMouseOver(Vector2f mousePos) override;
        bool isFocusLocked() const override;

        void setCursorType(CursorType type);

    private:
        Resources& resources;
        Vector2f value;
        bool held = false;
        CursorType cursorType = CursorType::Circle;
        Sprite cursor;
        Callback callback;

        static Sprite makeSprite(Resources& resources, Vector2f size, const String& material);
    };
}
