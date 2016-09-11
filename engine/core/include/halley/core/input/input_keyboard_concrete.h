#pragma once

#include <deque>
#include "input_button_base.h"
#include "input_keyboard.h"
struct SDL_KeyboardEvent;

namespace Halley {

#ifdef _MSC_VER
#pragma warning(disable: 4250)
#endif

	class InputKeyboardConcrete : public InputButtonBase, public InputKeyboard {
	public:
		int getNextLetter() override;
		String getButtonName(int code) override;

	private:
		std::deque<int> letters;

		InputKeyboardConcrete();

		void processEvent(const SDL_Event &event);
		void onTextEntered(const char* text);

		friend class Input;
	};

#ifdef _MSC_VER
#pragma warning(default: 4250)
#endif

}
