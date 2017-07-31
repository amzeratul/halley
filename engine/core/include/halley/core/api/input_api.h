#pragma once

#include <functional>
#include "halley/maths/vector2.h"
#include "halley/core/input/input_device.h"
#include "halley/maths/colour.h"
#include "halley/data_structures/maybe.h"

namespace Halley
{
	class InputJoystick;
	class InputTouch;

	class InputControllerData {
	public:
		Colour colour;
		String name;
	};

	class InputAPI
	{
	public:
		virtual ~InputAPI() {}
		
		virtual size_t getNumberOfKeyboards() const = 0;
		virtual std::shared_ptr<InputDevice> getKeyboard(int id = 0) const = 0;

		virtual size_t getNumberOfJoysticks() const = 0;
		virtual std::shared_ptr<InputJoystick> getJoystick(int id = 0) const = 0;

		virtual size_t getNumberOfMice() const = 0;
		virtual std::shared_ptr<InputDevice> getMouse(int id = 0) const = 0;

		virtual Vector<std::shared_ptr<InputTouch>> getNewTouchEvents() = 0;
		virtual Vector<std::shared_ptr<InputTouch>> getTouchEvents() = 0;

		virtual void setMouseRemapping(std::function<Vector2f(Vector2i)> remapFunction) = 0;

		virtual void requestControllerSetup(int minControllers, int maxControllers, std::function<void(bool)> callback, Maybe<std::vector<InputControllerData>> controllerData = {}) { callback(true); }
	};
}
