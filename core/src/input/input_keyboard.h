/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#pragma once

#include "input_button_base.h"
#include "input_keys.h"
#include <deque>
struct SDL_KeyboardEvent;

namespace Halley {
	class InputKeyboard : public virtual InputDevice {
	public:
		virtual ~InputKeyboard() {}
		virtual int getNextLetter()=0;
	};

	using spInputKeyboard = std::shared_ptr<InputKeyboard>;

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

	typedef std::shared_ptr<InputKeyboardConcrete> spInputKeyboardConcrete;

}
