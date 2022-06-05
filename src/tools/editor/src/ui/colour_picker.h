#pragma once

#include <halley/ui/halley_ui.h>

namespace Halley {
	class ColourPickerDisplay;

	class ColourPickerButton : public UIImage {
    public:
        using Callback = std::function<void(Colour4f, bool)>;

    	ColourPickerButton(UIFactory& factory, Colour4f colour, Callback callback);

    	void setColour(Colour4f colour, bool final);

    protected:
        void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;

    private:
        UIFactory& factory;
        Callback callback;
    };

    class ColourPicker : public UIWidget {
    public:
        using Callback = std::function<void(Colour4f, bool)>;

    	ColourPicker(UIFactory& factory, Colour4f initialColour, Callback callback);

        Colour4f getColour() const;
        void setColour(Colour4f col);

        void update(Time t, bool moved) override;

    private:
        Colour4f initialColour;
        Colour4f colour;
        Callback callback;
        std::shared_ptr<ColourPickerDisplay> mainDisplay;
        std::shared_ptr<ColourPickerDisplay> ribbonDisplay;

        void onMakeUI() override;

        void accept();
        void cancel();
        void onColourChanged();
    };

    class ColourPickerDisplay : public UIImage {
    public:
        enum class CursorType {
	        Circle,
            HorizontalLine
        };

        ColourPickerDisplay(String id, Vector2f size, Resources& resources, const String& material);

        void update(Time t, bool moved) override;
        void draw(UIPainter& painter) const override;

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

        static Sprite makeSprite(Resources& resources, Vector2f size, const String& material);
    };
}
