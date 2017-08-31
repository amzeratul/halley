#pragma once

#include <array>

namespace Halley {
	namespace UIInput {
		enum class Button {
			Accept,
			Cancel,
			Next,
			Prev,
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
		int xAxis = -1;
		int yAxis = -1;
		int xAxisAlt = -1;
		int yAxisAlt = -1;
	};

	class UIInputResults {
	public:
		UIInputResults();
		UIInputResults(std::array<bool, UIInput::NumberOfButtons> buttons, std::array<int8_t, UIInput::NumberOfAxes> axes);

		void reset();

		bool isButtonPressed(UIInput::Button button) const;
		int getAxisRepeat(UIInput::Axis axis) const;
		
		void setButtonPressed(UIInput::Button button, bool pressed);
		void setAxisRepeat(UIInput::Axis axis, int value);

	private:
		std::array<bool, UIInput::NumberOfButtons> buttons;
		std::array<int8_t, UIInput::NumberOfAxes> axes;
	};
}
