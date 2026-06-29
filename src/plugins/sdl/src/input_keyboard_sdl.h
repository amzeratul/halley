#pragma once

#include <set>
#include "halley/input/input_keyboard.h"
#include "halley/api/clipboard.h"
struct SDL_KeyboardEvent;

namespace Halley {

	class InputKeyboardSDL;

	class InputKeyboardSDL final : public InputKeyboard {
	public:
		String getButtonName(int code) const override;

		void update();

	private:
		InputKeyboardSDL(std::shared_ptr<IClipboard> clipboard);

		void processEvent(const SDL_Event &event);
		void onFocusLost();

		void setupMapping();
		KeyCode getHalleyKeyCodeFromSDLVirtualKeyCode(int sdlKeyCode) const;
		KeyMods getMods(int sdlMods) const;

		HashMap<int16_t, KeyCode> virtualKeyCodeToHalley;

		friend class InputSDL;
	};
	
}
