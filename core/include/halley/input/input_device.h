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

union SDL_Event;

namespace Halley {

	class InputDevice {
	public:
		virtual ~InputDevice() {}

		virtual bool isEnabled() const=0;

		virtual size_t getNumberButtons()=0;;
		virtual size_t getNumberAxes()=0;
		virtual size_t getNumberHats()=0;

		virtual String getButtonName(int code)=0;

		virtual bool isAnyButtonPressed()=0;
		virtual bool isButtonPressed(int code)=0;
		virtual bool isButtonPressedRepeat(int code)=0;
		virtual bool isButtonReleased(int code)=0;
		virtual bool isButtonDown(int code)=0;

		virtual void clearButton(int code)=0;

		virtual float getAxis(int n)=0;
		virtual int getAxisRepeat(int n)=0;
		virtual InputDevice& getHat(int n)=0;

		virtual void vibrate(spInputVibration vib)=0;
		virtual void stopVibrating()=0;
	};
	
}
