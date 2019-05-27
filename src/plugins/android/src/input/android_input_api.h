#pragma once

#include "halley/core/api/halley_api_internal.h"
#include "halley/core/input/input_touch.h"
#include <vector>
#include <memory>
#include <android/input.h>
#include "android_mouse.h"

namespace Halley {
    class AndroidSystemAPI;

    class AndroidInputAPI : public InputAPIInternal {
    public:
        AndroidInputAPI(AndroidSystemAPI& system);

        void init() override;
        void deInit() override;

        void beginEvents(Time t) override;

        size_t getNumberOfKeyboards() const override;
        std::shared_ptr<InputKeyboard> getKeyboard(int id) const override;

        size_t getNumberOfJoysticks() const override;
        std::shared_ptr<InputJoystick> getJoystick(int id) const override;

        size_t getNumberOfMice() const override;
        std::shared_ptr<InputDevice> getMouse(int id) const override;

        std::vector<std::shared_ptr<InputTouch>> getNewTouchEvents() override;
        std::vector<std::shared_ptr<InputTouch>> getTouchEvents() override;
        void setMouseRemapping(std::function<Vector2f(Vector2i)> remapFunction) override;

        void onInputEvent(AInputEvent* event);

    private:
        AndroidSystemAPI& system;

        std::shared_ptr<AndroidMouse> mouse;
        std::function<Vector2f(Vector2i)> mouseRemap;
        bool firstTouch = false;

        std::map<int32_t, std::shared_ptr<InputTouch>> touches;

        void onMotionEvent(AInputEvent* event);
        void onKeyEvent(AInputEvent* event);
    };
}