#pragma once

#include "halley/api/halley_api_internal.h"
#include "halley/input/input_keyboard.h"
#include "halley/api/clipboard.h"

struct SDL_KeyboardEvent;

namespace Halley {

	class SystemSDL3;

	class InputKeyboardSDL3 final : public InputKeyboard {
	public:
		String getButtonName(int code) const override;

		void update();

		void removeCapture(ITextInputCapture* capture) override;

	protected:
		std::unique_ptr<ITextInputCapture> makeTextInputCapture() override;

	private:
		explicit InputKeyboardSDL3(SystemAPI& system, std::shared_ptr<IClipboard> clipboard);

		void processEvent(const SDL_Event &event);

		void setupMapping();
		KeyCode getHalleyKeyCodeFromSDLVirtualKeyCode(int sdlKeyCode) const;
		KeyMods getMods(int sdlMods) const;
		
		HashMap<int16_t, KeyCode> virtualKeyCodeToHalley;

		SystemSDL3& system;

		friend class InputSDL3;
	};
	
}
