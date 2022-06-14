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
	enum class KeyCode;

	enum class JoystickType {
		None,
		Generic,
		Xbox,
		Playstation,
		SwitchFull,
		SwitchLeftJoycon,
		SwitchRightJoycon
	};

	enum class DefaultInputButtons {
		Primary,
		Secondary,
		Accept,
		Cancel
	};

	template <>
	struct EnumNames<DefaultInputButtons> {
		constexpr std::array<const char*, 4> operator()() const {
			return{{
				"primary",
				"secondary",
				"accept",
				"cancel"
			}};
		}
	};

	using InputButton = int;
	
	class InputDevice {
	public:
		virtual ~InputDevice();

		virtual bool isEnabled() const;

		virtual size_t getNumberButtons();
		virtual size_t getNumberAxes();

		virtual String getButtonName(int code);

		virtual bool isAnyButtonPressed();
		virtual bool isAnyButtonPressedRepeat();
		virtual bool isAnyButtonReleased();
		virtual bool isAnyButtonDown();

		virtual bool isButtonPressed(InputButton code);
		virtual bool isButtonPressedRepeat(InputButton code);
		virtual bool isButtonReleased(InputButton code);
		virtual bool isButtonDown(InputButton code);

		virtual void clearButton(InputButton code);
		virtual void clearButtonPress(InputButton code);
		virtual void clearButtonRelease(InputButton code);

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
