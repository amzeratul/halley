#include "winrt_input.h"
using namespace Halley;

void WinRTInput::init()
{
	
}

void WinRTInput::deInit()
{
	
}

void WinRTInput::beginEvents(Time t)
{
	
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
	// TODO
	return 0;
}

std::shared_ptr<InputJoystick> WinRTInput::getJoystick(int id) const
{
	// TODO
	return {};
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
