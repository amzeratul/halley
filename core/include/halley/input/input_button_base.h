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

#include "input_device.h"
#include <halley/support/exception.h>

namespace Halley {
	class InputButtonBase : public virtual InputDevice {
	public:
		InputButtonBase(int nButtons=-1);

		size_t getNumberButtons() override { return buttonDown.size(); }

		bool isAnyButtonPressed() override;
		bool isButtonPressed(int code) override;
		bool isButtonPressedRepeat(int code) override;
		bool isButtonReleased(int code) override;
		bool isButtonDown(int code) override;

		void clearButton(int code) override;

		virtual String getButtonName(int code) override;

		virtual bool isEnabled() const override { return true; }

		virtual size_t getNumberAxes() override { return 0; }
		virtual size_t getNumberHats() override { return 0; }

		virtual float getAxis(int /*n*/) override {return 0; };
		virtual int getAxisRepeat(int /*n*/) override { return 0; }
		virtual InputDevice& getHat(int /*n*/) override { throw Exception("Hat not available."); }

		virtual void vibrate(spInputVibration /*vib*/) override {}
		virtual void stopVibrating() override {}

	protected:
		std::vector<char> buttonPressed;
		std::vector<char> buttonPressedRepeat;
		std::vector<char> buttonReleased;
		std::vector<char> buttonDown;

		void init(int nButtons);

		void onButtonPressed(int code);
		void onButtonReleased(int code);
		void onButtonStatus(int code, bool down);
		void clearPresses();

		friend class InputJoystickXInput;
	};

	typedef std::shared_ptr<InputButtonBase> spInputButtonBase;
}
