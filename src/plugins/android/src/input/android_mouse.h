#pragma once

#include <halley/core/input/input_button_base.h>

namespace Halley {
    class AndroidMouse : public InputButtonBase {
    public:
        AndroidMouse();

        void setPosition(Vector2f pos);

        Vector2f getPosition() const override;

    private:
        Vector2f pos;
    };
}
