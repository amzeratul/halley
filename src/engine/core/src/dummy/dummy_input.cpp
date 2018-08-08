#include "dummy_input.h"

using namespace Halley;

size_t DummyInputAPI::getNumberOfKeyboards() const
{
	return 0;
}

std::shared_ptr<InputKeyboard> DummyInputAPI::getKeyboard(int id) const
{
	return {};
}

size_t DummyInputAPI::getNumberOfJoysticks() const
{
	return 0;
}

std::shared_ptr<InputJoystick> DummyInputAPI::getJoystick(int id) const
{
	return {};
}

size_t DummyInputAPI::getNumberOfMice() const
{
	return 0;
}

std::shared_ptr<InputDevice> DummyInputAPI::getMouse(int id) const
{
	return {};
}

Vector<std::shared_ptr<InputTouch>> DummyInputAPI::getNewTouchEvents()
{
	return {};
}

Vector<std::shared_ptr<InputTouch>> DummyInputAPI::getTouchEvents()
{
	return {};
}

void DummyInputAPI::setMouseRemapping(std::function<Vector2f(Vector2i)> remapFunction)
{
}

void DummyInputAPI::init()
{
}

void DummyInputAPI::deInit()
{
}

void DummyInputAPI::beginEvents(Time t)
{
}
