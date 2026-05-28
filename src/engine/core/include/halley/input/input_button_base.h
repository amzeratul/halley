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
		InputButtonBase(int nButtons = -1);

		size_t getNumberButtons() const override { return buttonDown.size(); }

		bool isAnyButtonPressed() const override;
		bool isAnyButtonPressedRepeat() const override;
		bool isAnyButtonReleased() const override;
		bool isAnyButtonDown() const override;

		bool isButtonPressed(InputButton code) const override;
		bool isButtonPressedRepeat(InputButton code) const override;
		bool isButtonReleased(InputButton code) const override;
		bool isButtonDown(InputButton code) const override;

		bool isButtonPressed(KeyCode code) const;
		bool isButtonPressedRepeat(KeyCode code) const;
		bool isButtonReleased(KeyCode code) const;
		bool isButtonDown(KeyCode code) const;

		void clearButton(InputButton code) override;
		void clearButtonPress(InputButton code) override;
		void clearButtonRelease(InputButton code) override;

		String getButtonName(int code) const override;

		void clearPresses() override;

		void onButtonStatus(int code, bool down);

		void setParent(const std::shared_ptr<InputDevice>& parent) override;
		std::shared_ptr<InputDevice> getParent() const override;

	protected:
		Vector<char> buttonPressed;
		Vector<char> buttonPressedRepeat;
		Vector<char> buttonReleased;
		Vector<char> buttonDown;
		bool anyButtonPressed = false;
		bool anyButtonPressedRepeat = false;
		bool anyButtonReleased = false;
		std::weak_ptr<InputDevice> parent;

		void init(int nButtons);

		void onButtonPressed(int code);
		void onButtonReleased(int code);
		virtual void onButtonsCleared();
	};

	class InputMouse : public InputButtonBase {
	public:
		InputType getInputType() const override;
		std::string_view getName() const override;

		String getButtonName(int code) const override;
	};

	typedef std::shared_ptr<InputButtonBase> spInputButtonBase;
}
