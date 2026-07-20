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

#include "input_keys.h"
#include "halley/maths/colour.h"
#include "halley/maths/quaternion.h"
#include "halley/maths/vector2.h"

namespace Halley {
	class InputExclusiveButton;

	enum class InputType : uint8_t {
		None,
		Keyboard,
		Mouse,
		Gamepad,
		Virtual
	};

	template <>
	struct EnumNames<InputType> {
		constexpr auto operator()() const {
			return std::to_array({
				"none",
				"keyboard",
				"mouse",
				"gamepad",
				"virtual"
			});
		}
	};
	
	enum class InputSubType : uint8_t {
		Button,
		Axis
	};

	template <>
	struct EnumNames<InputSubType> {
		constexpr auto operator()() const {
			return std::to_array({
				"button",
				"axis"
			});
		}
	};

	enum class JoystickType : uint8_t {
		None,
		Generic,
		Xbox,
		Playstation,
		SwitchFull,
		SwitchLeftJoycon,
		SwitchRightJoycon
	};

	template <>
	struct EnumNames<JoystickType> {
		constexpr auto operator()() const {
			return std::to_array({
				"none",
				"generic",
				"xbox",
				"playstation",
				"switchFull",
				"switchLeftJoycon",
				"switchRightJoycon"
			});
		}
	};

	enum class DefaultInputButtons : uint8_t {
		Primary,
		Secondary,
		Accept,
		Cancel
	};

	template <>
	struct EnumNames<DefaultInputButtons> {
		constexpr auto operator()() const {
			return std::to_array({
				"primary",
				"secondary",
				"accept",
				"cancel"
			});
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
		constexpr auto operator()() const {
			return std::to_array({
                "minimum",
				"low",
                "normal",
				"high",
                "maximum"
			});
		}
	};

	enum class JoystickButtonPosition : uint8_t
	{
		FaceBottom,
		FaceRight,
		FaceLeft,
		FaceTop,
		BumperLeft,
		BumperRight,
		TriggerLeft,
		TriggerRight,
		LeftStick,
		RightStick,
		Select,
		Start,
		DPadUp,
		DPadRight,
		DPadDown,
		DPadLeft,
		System,
		Misc1,
		TouchPad,
		Paddle1,
		Paddle2,
		Paddle3,
		Paddle4,
		Accept,
		Cancel,
		COUNT
	};

	template <>
	struct EnumNames<JoystickButtonPosition> {
		constexpr auto operator()() const {
			return std::to_array({
				"faceBottom",
				"faceRight",
				"faceLeft",
				"faceTop",
				"bumperLeft",
				"bumperRight",
				"triggerLeft",
				"triggerRight",
				"leftStick",
				"rightStick",
				"select",
				"start",
				"dPadUp",
				"dPadRight",
				"dPadDown",
				"dPadLeft",
				"system",
				"misc1",
				"touchPad",
				"paddle1",
				"paddle2",
				"paddle3",
				"paddle4",
				"accept",
				"cancel",
				"COUNT"
			});
		}
	};

	enum class JoystickAxisPosition : uint8_t
	{
		LeftStickX,
		LeftStickY,
		RightStickX,
		RightStickY,
		TriggerLeft,
		TriggerRight
	};

	template <>
	struct EnumNames<JoystickAxisPosition> {
		constexpr auto operator()() const {
			return std::to_array({
				"leftStickX",
				"leftStickY",
				"rightStickX",
				"rightStickY",
				"triggerLeft",
				"triggerRight"
			});
		}
	};

	enum class JoystickAxisDirection : uint8_t {
		Positive,
		Negative
	};
	
	template <>
	struct EnumNames<JoystickAxisDirection> {
		constexpr auto operator()() const {
			return std::to_array({
				"positive",
				"negative"
			});
		}
	};

	enum class MouseButton : uint8_t {
		Left,
		Middle,
		Right,
		B4,
		B5,
		WheelUp,
		WheelDown,
		WheelUpDown,
		WheelLeft,
		WheelRight,
		WheelLeftRight
	};

	template <>
	struct EnumNames<MouseButton> {
		constexpr auto operator()() const {
			return std::to_array({
				"left",
				"middle",
				"right",
				"b4",
				"b5",
				"wheelUp",
				"wheelDown",
				"wheelUpDown",
				"wheelLeft",
				"wheelRight",
				"wheelLeftRight"
			});
		}
	};

	using InputButton = int;

	class InputMotionSensor {
	public:
		Vector3f acceleration;
		Quaternion rotation;

		Vector3f velocity; // Integrated from acceleration

		Angle1f angleX;
		Angle1f angleY;
		Angle1f angleZ;
		Vector3f angularVelocities;

		Time dt = 0;
		int64_t sampleNumber = 0;
	};
	
	class InputDevice {
	public:
		InputDevice();
		virtual ~InputDevice();

		uint16_t getId() const { return deviceId; }
		virtual std::string_view getName() const;

		virtual bool isEnabled() const;

		virtual size_t getNumberButtons() const;
		virtual size_t getNumberAxes() const;
		virtual size_t getNumberMotionSensors() const;

		virtual String getButtonName(int code) const;
		virtual String getAxisName(int index) const;
		virtual int getButtonAtPosition(JoystickButtonPosition position) const;
		virtual int getAxisAtPosition(JoystickAxisPosition position) const;
		virtual std::optional<JoystickButtonPosition> getPositionForButton(int code) const;
		virtual std::optional<JoystickAxisPosition> getPositionForAxis(int code) const;

		virtual bool isAnyButtonPressed() const;
		virtual bool isAnyButtonPressedRepeat() const;
		virtual bool isAnyButtonReleased() const;
		virtual bool isAnyButtonDown() const;

		virtual bool isButtonPressed(InputButton code) const;
		virtual bool isButtonPressedRepeat(InputButton code) const;
		virtual bool isButtonReleased(InputButton code) const;
		virtual bool isButtonDown(InputButton code) const;
		virtual KeyMods getKeyMods() const;

		virtual bool hasAnyInput() const;

		virtual void clearButton(InputButton code);
		virtual void clearButtonPress(InputButton code);
		virtual void clearButtonRelease(InputButton code);
		virtual void clearPresses();
		virtual void clearAxes();

		virtual float getAxis(int n) const;
		virtual int getAxisRepeat(int n) const;

		virtual size_t getNumberHats() const;
		virtual std::shared_ptr<InputDevice> getHat(int n) const;

		virtual std::pair<float, float> getVibration() const;
		virtual void setVibration(float low, float high);
		virtual void vibrate(spInputVibration vib);
		virtual void stopVibrating();
		
		virtual JoystickType getJoystickType() const;
		virtual InputType getInputType() const;

		virtual Vector2f getPosition() const;
		virtual void setPosition(Vector2f position);
		virtual Vector2f getWheelMove() const;
		virtual Vector2i getWheelMoveDiscrete() const;

		virtual const InputMotionSensor& getMotionSensor(int n) const;

		virtual void setParent(const std::shared_ptr<InputDevice>& parent);
		virtual std::shared_ptr<InputDevice> getParent() const;

		virtual bool isManual() const;

		virtual std::optional<int> getPlayerIndex() const;
		virtual bool hasLED() const;
		virtual void setLED(Colour4c colour) const;

		Vector<int> getButtonsPressed() const;
		Vector<int> getButtonsReleased() const;
		Vector<int> getButtonsDown() const;
		Vector<std::pair<int, JoystickAxisDirection>> getAxesMoved(float threshold = 0.5f) const;

	private:
		uint16_t deviceId = 0;
	};

	class InputAxisRepeater {
	public:
		InputAxisRepeater(Time firstDelay = 0.2, Time repeatInterval0 = 0.1, Time secondDelay = std::numeric_limits<float>::max(), Time repeatInterval1 = 0);
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
