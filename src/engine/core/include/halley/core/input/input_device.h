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

#include "input_vibration.h"
#include <halley/text/halleystring.h>
#include "halley/maths/vector2.h"

namespace Halley {

	enum class JoystickType {
		None,
		Generic,
		Xbox,
		Playstation,
		SwitchFull,
		SwitchLeftJoycon,
		SwitchRightJoycon
	};
	
	class InputDevice {
	public:
		virtual ~InputDevice();

		virtual bool isEnabled() const;

		virtual size_t getNumberButtons();
		virtual size_t getNumberAxes();

		virtual String getButtonName(int code);

		virtual bool isAnyButtonPressed();
		virtual bool isAnyButtonReleased();
		virtual bool isAnyButtonDown();

		virtual bool isButtonPressed(int code);
		virtual bool isButtonPressedRepeat(int code);
		virtual bool isButtonReleased(int code);
		virtual bool isButtonDown(int code);

		virtual void clearButton(int code);
		virtual void clearButtonPress(int code);
		virtual void clearButtonRelease(int code);

		virtual float getAxis(int /*n*/);
		virtual int getAxisRepeat(int /*n*/);

		virtual size_t getNumberHats();
		virtual std::shared_ptr<InputDevice> getHat(int /*n*/);

		virtual void vibrate(spInputVibration /*vib*/);
		virtual void stopVibrating();
		virtual JoystickType getJoystickType() const;

		virtual Vector2f getPosition() const;
		virtual int getWheelMove() const;

		virtual void setParent(InputDevice* parent);
		virtual InputDevice* getParent() const;
	};
	
}
