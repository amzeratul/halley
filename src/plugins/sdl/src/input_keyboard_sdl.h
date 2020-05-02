#pragma once

#include <set>
#include "halley/core/input/input_keyboard.h"
#include "halley/core/api/clipboard.h"
struct SDL_KeyboardEvent;

namespace Halley {

	class InputKeyboardSDL;

	class InputKeyboardSDL final : public InputKeyboard {
	public:
		String getButtonName(int code) override;

		void update();

	private:
		InputKeyboardSDL(std::shared_ptr<IClipboard> clipboard);

		void processEvent(const SDL_Event &event);

		friend class InputSDL;
	};
	
}
