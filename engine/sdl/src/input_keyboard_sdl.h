#pragma once

#include <deque>
#include "halley/core/input/input_button_base.h"
#include "halley/core/input/input_keyboard.h"
struct SDL_KeyboardEvent;

namespace Halley {

#ifdef _MSC_VER
#pragma warning(disable: 4250)
#endif

	class InputKeyboardSDL : public InputButtonBase, public InputKeyboard {
	public:
		int getNextLetter() override;
		String getButtonName(int code) override;

	private:
		std::deque<int> letters;

		InputKeyboardSDL();

		void processEvent(const SDL_Event &event);
		void onTextEntered(const char* text);

		friend class InputSDL;
	};

#ifdef _MSC_VER
#pragma warning(default: 4250)
#endif

}
