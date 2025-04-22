#include <SDL3/SDL.h>
#include "input_mouse_sdl3.h"

using namespace Halley;

InputMouseSDL3::InputMouseSDL3()
{
	init(11);
}

void InputMouseSDL3::processEvent(const SDL_Event& event, const std::function<Vector2f(Vector2i)>& remap)
{
	switch (event.type) {
	case SDL_EVENT_MOUSE_MOTION:
		{
			Vector2i p;
			p.x = int(event.motion.x);
			p.y = int(event.motion.y);
			relMove = Vector2f(event.motion.xrel, event.motion.yrel);
			pos = remap(Vector2i(p));
			break;
		}

	case SDL_EVENT_MOUSE_BUTTON_DOWN:
		{
			Uint8 button = event.button.button;
			onButtonPressed(button - 1);
			break;
		}

	case SDL_EVENT_MOUSE_BUTTON_UP:
		{
			Uint8 button = event.button.button;
			onButtonReleased(button - 1);
			break;
		}

	case SDL_EVENT_MOUSE_WHEEL:
		{
			wheelMove += Vector2f(event.wheel.x, event.wheel.y);
			wheelMoveDiscrete += Vector2i(int(event.wheel.x), int(event.wheel.y));

            if (event.wheel.y > 0) {
				onButtonStatus(static_cast<int>(MouseButton::WheelUp), true);
				onButtonStatus(static_cast<int>(MouseButton::WheelUpDown), true);
			} else if (event.wheel.y < 0) {
				onButtonStatus(static_cast<int>(MouseButton::WheelDown), true);
				onButtonStatus(static_cast<int>(MouseButton::WheelUpDown), true);
			}

			if (event.wheel.x > 0) {
				onButtonStatus(static_cast<int>(MouseButton::WheelLeft), true);
				onButtonStatus(static_cast<int>(MouseButton::WheelLeftRight), true);
			} else if (event.wheel.x < 0) {
				onButtonStatus(static_cast<int>(MouseButton::WheelRight), true);
				onButtonStatus(static_cast<int>(MouseButton::WheelLeftRight), true);
			}
		}
	}
}

void InputMouseSDL3::updateRemap(const std::function<Vector2f(Vector2i)>& remap) {
	float x, y;
	SDL_GetMouseState(&x, &y);
	pos = remap(Vector2i(int(x), int(y)));
}

Vector2f InputMouseSDL3::getPosition() const
{
	return pos;
}

void InputMouseSDL3::setPosition(Vector2f position)
{
	SDL_WarpMouseInWindow(nullptr, position.x, position.y);
}

Vector2f InputMouseSDL3::getWheelMove() const
{
	return wheelMove;
}

Vector2i InputMouseSDL3::getWheelMoveDiscrete() const
{
	return wheelMoveDiscrete;
}

void InputMouseSDL3::update()
{
	clearPresses();
	onButtonStatus(static_cast<int>(MouseButton::WheelUp), false);
	onButtonStatus(static_cast<int>(MouseButton::WheelDown), false);
	onButtonStatus(static_cast<int>(MouseButton::WheelUpDown), false);
	onButtonStatus(static_cast<int>(MouseButton::WheelLeft), false);
	onButtonStatus(static_cast<int>(MouseButton::WheelRight), false);
	onButtonStatus(static_cast<int>(MouseButton::WheelLeftRight), false);
}

InputType InputMouseSDL3::getInputType() const
{
	return InputType::Mouse;
}

std::string_view InputMouseSDL3::getName() const
{
	return "Mouse";
}

float InputMouseSDL3::getAxis(int n)
{
	if (n == 0) {
		return relMove.x;
		//return pos.x - prevPos.x;
	} else if (n == 1) {
		return relMove.y;
		//return pos.y - prevPos.y;
	} else if (n == 2) {
		return pos.x;
	} else if (n == 3) {
		return pos.y;
	} else {
		return 0.0f;
	}
}

void InputMouseSDL3::clearPresses()
{
	InputButtonBase::clearPresses();
	wheelMove = {};
	wheelMoveDiscrete = {};
	if (!isMouseTrapped) {
		prevPos = pos;
	}
	relMove = {};
}

void InputMouseSDL3::setDeltaPos(Vector2i deltaPos)
{
	this->prevPos = Vector2f((float)deltaPos.x, (float)deltaPos.y);
}

void InputMouseSDL3::setMouseTrapped(bool isTrapped)
{
	isMouseTrapped = isTrapped;
}