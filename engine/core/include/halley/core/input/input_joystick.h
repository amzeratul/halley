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

#include <functional>
#include "input_button_base.h"

#ifdef _MSC_VER
#define XINPUT_AVAILABLE
#endif

namespace Halley {

	enum class JoystickType {
		Generic,
		Xbox,
		Playstation
	};

	enum class JoystickButtonPosition
	{
		FaceTop,
		FaceRight,
		FaceBottom,
		FaceLeft,
		BumperLeft,
		BumperRight,
		TriggerLeft,
		TriggerRight,
		LeftStick,
		RightStick,
		Select,
		Start,
		PlatformAcceptButton,
		PlatformCancelButton
	};

	class InputJoystick : public InputButtonBase {
	public:
		virtual ~InputJoystick() {}

		virtual std::string getName() const = 0;
		virtual JoystickType getType() const = 0;

		size_t getNumberAxes() override;
		size_t getNumberHats() override;

		float getAxis(int n) override;
		std::shared_ptr<InputDevice> getHat(int n) override;

		static float defaultAxisAdjust(float value);

		virtual int getButtonAtPosition(JoystickButtonPosition position) const;

		virtual void update(Time /*t*/) {}

	protected:
		Vector<float> axes;
		Vector<spInputButtonBase> hats;
		std::function<float (float)> axisAdjust;
	};

	typedef std::shared_ptr<InputJoystick> spInputJoystick;
}
