#pragma once

#include <halley/ui/halley_ui.h>

namespace Halley {
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

    private:
        Colour4f initialColour;
        Colour4f colour;
        Callback callback;

    	void onMakeUI() override;

        void accept();
        void cancel();
        void onColourChanged();
    };
}
