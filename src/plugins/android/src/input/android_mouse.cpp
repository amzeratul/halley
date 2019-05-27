#include "android_mouse.h"
using namespace Halley;

AndroidMouse::AndroidMouse()
    : InputButtonBase(4)
{
}

void AndroidMouse::setPosition(Vector2f pos)
{
    this->pos = pos;
}

Vector2f AndroidMouse::getPosition() const
{
    return pos;
}
