#include <halley/support/logger.h>
#include "android_input_api.h"
#include "../android_system.h"

using namespace Halley;

AndroidInputAPI::AndroidInputAPI(AndroidSystemAPI& system)
    : system(system)
{
    mouse = std::make_shared<AndroidMouse>();
}

void AndroidInputAPI::init()
{
    AndroidSystem::get().setInputAPI(this);
}

void AndroidInputAPI::deInit()
{
}

void AndroidInputAPI::beginEvents(Time t)
{
    mouse->onButtonStatus(0, false);
    mouse->onButtonStatus(1, false);
    mouse->onButtonStatus(2, holdingRight);
}

size_t AndroidInputAPI::getNumberOfKeyboards() const
{
    return 0;
}

std::shared_ptr<InputKeyboard> AndroidInputAPI::getKeyboard(int id) const
{
    return std::shared_ptr<InputKeyboard>();
}

size_t AndroidInputAPI::getNumberOfJoysticks() const
{
    return 0;
}

std::shared_ptr<InputJoystick> AndroidInputAPI::getJoystick(int id) const
{
    return std::shared_ptr<InputJoystick>();
}

size_t AndroidInputAPI::getNumberOfMice() const
{
    return 1;
}

std::shared_ptr<InputDevice> AndroidInputAPI::getMouse(int id) const
{
    return mouse;
}

Vector<std::shared_ptr<InputTouch>> AndroidInputAPI::getNewTouchEvents()
{
    // TODO
    return {};
}

Vector<std::shared_ptr<InputTouch>> AndroidInputAPI::getTouchEvents()
{
    // TODO
    return {};
}

void AndroidInputAPI::setMouseRemapping(std::function<Vector2f(Vector2i)> remapFunction)
{
    mouseRemap = remapFunction;
}

void AndroidInputAPI::onInputEvent(AInputEvent *event)
{
    const auto type = AInputEvent_getType(event);
    if (type == AINPUT_EVENT_TYPE_MOTION) {
        onMotionEvent(event);
    } else if (type == AINPUT_EVENT_TYPE_KEY) {
        onKeyEvent(event);
    }
}

void AndroidInputAPI::onMotionEvent(AInputEvent *event)
{
    const int32_t actionIndex = AMotionEvent_getAction(event);
    const int32_t action = actionIndex & 0xFF;
    const int32_t index = (actionIndex >> 8) & 0xFF;
    const auto rawPos = Vector2f(AMotionEvent_getX(event, 0), AMotionEvent_getY(event, 0));
    const auto pos = mouseRemap(Vector2i(rawPos.round()));
    const int32_t pointerId = AMotionEvent_getPointerId(event, index);

    const auto eventTime = AMotionEvent_getEventTime(event);
    const auto startTime = AMotionEvent_getDownTime(event);
    const Time heldTime = Time(eventTime - startTime) / 1'000'000'000.0;

    if (action == AMOTION_EVENT_ACTION_DOWN) {
        touches[pointerId] = std::make_shared<InputTouch>(pos);
        firstTouch = touches.size() == 1;
        holdingRight = false;
    } else {
        auto touch = touches[pointerId];
        touch->setPos(pos);

        const bool validTap = firstTouch && (pos - touch->getInitialPos()).length() < 20.0f;
        holdingRight = validTap && heldTime >= 0.3;

        if (action == AMOTION_EVENT_ACTION_UP) {
            mouse->setPosition(pos);
            holdingRight = false;

            if (validTap && heldTime < 0.3) {
                mouse->onButtonStatus(0, true);
            }

            touches.erase(pointerId);
        } else if (action == AMOTION_EVENT_ACTION_MOVE) {
            mouse->setPosition(pos);
        }
    }
}

void AndroidInputAPI::onKeyEvent(AInputEvent *event)
{

}
