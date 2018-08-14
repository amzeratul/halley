#pragma once

#include <array>
#include <cstdint>

namespace Halley {
	namespace UIInput {
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
		UIInput::Priority priorityLevel = UIInput::Priority::Normal;

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

		bool isButtonPressed(UIInput::Button button) const;
		bool isButtonReleased(UIInput::Button button) const;
		bool isButtonHeld(UIInput::Button button) const;
		float getAxis(UIInput::Axis axis) const;
		int getAxisRepeat(UIInput::Axis axis) const;
		
		void setButton(UIInput::Button button, bool pressed, bool released, bool held);
		void setAxis(UIInput::Axis axis, float value);
		void setAxisRepeat(UIInput::Axis axis, int value);

	private:
		std::array<bool, UIInput::NumberOfButtons> buttonsPressed;
		std::array<bool, UIInput::NumberOfButtons> buttonsReleased;
		std::array<bool, UIInput::NumberOfButtons> buttonsHeld;
		std::array<float, UIInput::NumberOfAxes> axes;
		std::array<int8_t, UIInput::NumberOfAxes> axesRepeat;
	};
}
