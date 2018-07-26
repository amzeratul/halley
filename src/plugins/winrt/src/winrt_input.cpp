#ifdef WINDOWS_STORE

#include "winrt_input.h"
#include "winrt_gamepad.h"
using namespace Halley;

void WinRTInput::init()
{
	for (int i = 0; i < 4; ++i) {
		gamepads.push_back(std::make_shared<WinRTGamepad>(i));
	}
}

void WinRTInput::deInit()
{
	
}

void WinRTInput::beginEvents(Time t)
{
	for (auto& g: gamepads) {
		g->clearPresses();
		g->update(t);
	}
}

size_t WinRTInput::getNumberOfKeyboards() const
{
	return 0;
}

std::shared_ptr<InputDevice> WinRTInput::getKeyboard(int id) const
{
	return {};
}

size_t WinRTInput::getNumberOfJoysticks() const
{
	return 4;
}

std::shared_ptr<InputJoystick> WinRTInput::getJoystick(int id) const
{
	return gamepads.at(id);
}

size_t WinRTInput::getNumberOfMice() const
{
	return 0;
}

std::shared_ptr<InputDevice> WinRTInput::getMouse(int id) const
{
	return {};
}

void WinRTInput::setMouseRemapping(std::function<Vector2f(Vector2i)> remapFunction)
{
}

Vector<std::shared_ptr<InputTouch>> WinRTInput::getNewTouchEvents()
{
	return {};
}

Vector<std::shared_ptr<InputTouch>> WinRTInput::getTouchEvents()
{
	return {};
}

#endif