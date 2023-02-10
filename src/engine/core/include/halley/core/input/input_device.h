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
#include <halley/text/string_converter.h>
#include "halley/maths/vector2.h"

namespace Halley {
	enum class KeyCode;
	class InputExclusiveButton;

	enum class InputType {
		None,
		Keyboard,
		Mouse,
		Gamepad,
		Virtual
	};

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

	enum class InputPriority : int8_t {
        Minimum,
        Low,
        Normal,
        High,
        Maximum
    };

	template <>
	struct EnumNames<InputPriority> {
		constexpr std::array<const char*, 5> operator()() const {
			return{{
                "minimum",
				"low",
                "normal",
				"high",
                "maximum"
			}};
		}
	};

	using InputButton = int;
	
	class InputDevice {
	public:
		InputDevice();
		virtual ~InputDevice();

		uint16_t getId() const { return deviceId; }

		virtual bool isEnabled() const;

		virtual size_t getNumberButtons();
		virtual size_t getNumberAxes();

		virtual String getButtonName(int code) const;

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
		virtual void clearPresses();

		virtual float getAxis(int /*n*/);
		virtual int getAxisRepeat(int /*n*/);

		virtual size_t getNumberHats();
		virtual std::shared_ptr<InputDevice> getHat(int /*n*/);

		virtual std::pair<float, float> getVibration() const;
		virtual void setVibration(float low, float high);
		virtual void vibrate(spInputVibration /*vib*/);
		virtual void stopVibrating();
		
		virtual JoystickType getJoystickType() const;
		virtual InputType getInputType() const;

		virtual Vector2f getPosition() const;
		virtual void setPosition(Vector2f position);
		virtual int getWheelMove() const;

		virtual void setParent(InputDevice* parent);
		virtual InputDevice* getParent() const;

	private:
		uint16_t deviceId = 0;
	};

	class InputAxisRepeater {
	public:
		InputAxisRepeater(Time firstDelay = 0.2, Time repeatInterval0 = 0.1, Time secondDelay = std::numeric_limits<Time>::infinity(), Time repeatInterval1 = 0);
		int update(float value, Time t);

	private:
		Time firstDelay;
		Time secondDelay;
		Time repeatInterval0;
		Time repeatInterval1;

		int lastValue = 0;
		Time timeHeld = 0;
		Time timeSinceLastRepeat;
	};
	
}
