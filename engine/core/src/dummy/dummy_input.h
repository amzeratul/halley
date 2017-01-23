#pragma once
#include "api/halley_api_internal.h"

namespace Halley {
	class DummyInputAPI : public InputAPIInternal {
	public:
		size_t getNumberOfKeyboards() const override;
		std::shared_ptr<InputKeyboard> getKeyboard(int id) const override;
		size_t getNumberOfJoysticks() const override;
		std::shared_ptr<InputJoystick> getJoystick(int id) const override;
		size_t getNumberOfMice() const override;
		std::shared_ptr<InputMouse> getMouse(int id) const override;
		Vector<std::shared_ptr<InputTouch>> getNewTouchEvents() override;
		Vector<std::shared_ptr<InputTouch>> getTouchEvents() override;
		void setMouseRemapping(std::function<Vector2f(Vector2i)> remapFunction) override;
		void init() override;
		void deInit() override;
		void beginEvents(Time t) override;
	};
}
