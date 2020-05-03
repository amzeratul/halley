#pragma once

#include <array>
#include <cstdint>

namespace Halley {
	namespace UIGamepadInput {
		enum class Button {
			Accept,
			Cancel,
			Next,
			Prev,
			Hold,
			NumberOfButtons
		};

		enum class Axis {
			X,
			Y,
			NumberOfAxes
		};

		enum class Priority {
			None,
			Lowest,
			Normal,
			Focused
		};

		constexpr static int NumberOfButtons = int(Button::NumberOfButtons);
		constexpr static int NumberOfAxes = int(Axis::NumberOfAxes);
	}

	class UIInputButtons {
    public:
		UIGamepadInput::Priority priorityLevel = UIGamepadInput::Priority::Normal;

		int accept = -1;
		int cancel = -1;
		int next = -1;
		int prev = -1;
		int hold = -1;
		int xAxis = -1;
		int yAxis = -1;
		int xAxisAlt = -1;
		int yAxisAlt = -1;
	};

	class UIInputResults {
	public:
		UIInputResults();

		void reset();

		bool isButtonPressed(UIGamepadInput::Button button) const;
		bool isButtonReleased(UIGamepadInput::Button button) const;
		bool isButtonHeld(UIGamepadInput::Button button) const;
		float getAxis(UIGamepadInput::Axis axis) const;
		int getAxisRepeat(UIGamepadInput::Axis axis) const;
		
		void setButton(UIGamepadInput::Button button, bool pressed, bool released, bool held);
		void setAxis(UIGamepadInput::Axis axis, float value);
		void setAxisRepeat(UIGamepadInput::Axis axis, int value);

	private:
		std::array<bool, UIGamepadInput::NumberOfButtons> buttonsPressed;
		std::array<bool, UIGamepadInput::NumberOfButtons> buttonsReleased;
		std::array<bool, UIGamepadInput::NumberOfButtons> buttonsHeld;
		std::array<float, UIGamepadInput::NumberOfAxes> axes;
		std::array<int8_t, UIGamepadInput::NumberOfAxes> axesRepeat;
	};
}
