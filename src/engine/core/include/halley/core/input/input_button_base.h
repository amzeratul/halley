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
	class InputButtonBase : public InputDevice {
	public:
		InputButtonBase(int nButtons=-1);

		size_t getNumberButtons() override { return buttonDown.size(); }

		bool isAnyButtonPressed() override;
		bool isAnyButtonReleased() override;
		bool isAnyButtonDown() override;

		bool isButtonPressed(int code) override;
		bool isButtonPressedRepeat(int code) override;
		bool isButtonReleased(int code) override;
		bool isButtonDown(int code) override;

		void clearButton(int code) override;
		void clearButtonPress(int code) override;
		void clearButtonRelease(int code) override;

		String getButtonName(int code) override;

		void clearPresses();

		void onButtonStatus(int code, bool down);

		void setParent(InputDevice* parent) override;
		InputDevice* getParent() const override;

	protected:
		Vector<char> buttonPressed;
		Vector<char> buttonPressedRepeat;
		Vector<char> buttonReleased;
		Vector<char> buttonDown;
		InputDevice* parent = nullptr;

		void init(int nButtons);

		virtual void onButtonPressed(int code);
		virtual void onButtonReleased(int code);
	};

	typedef std::shared_ptr<InputButtonBase> spInputButtonBase;
}
