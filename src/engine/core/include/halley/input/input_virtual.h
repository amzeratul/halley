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
#include "input_exclusive.h"
#include "halley/maths/rect.h"
#include "halley/data_structures/maybe.h"
#include <set>

#include "input_keys.h"
#include "halley/data_structures/hash_map.h"
#include "halley/utils/convertible_to.h"

namespace Halley {
	enum class KeyCode : uint16_t;
	using spInputDevice = std::shared_ptr<InputDevice>;
	class InputExclusiveButton;
	class InputExclusiveAxis;

	class InputVirtual final : public InputDevice {
		friend class InputExclusiveButton;
		friend class InputExclusiveAxis;

	public:
		InputVirtual(int nButtons, int nAxes, InputType type = InputType::Virtual);
		~InputVirtual() override;

		bool isEnabled() const override;
		size_t getNumberHats() const override;
		std::shared_ptr<InputDevice> getHat(int /*n*/) const override;

		size_t getNumberButtons() const override;
		size_t getNumberAxes() const override;
		size_t getNumberMotionSensors() const override;

		bool isAnyButtonPressed() const override;
		bool isAnyButtonReleased() const override;
		bool isAnyButtonDown() const override;

		bool isButtonPressed(InputButton code) const override;
		bool isButtonPressedRepeat(InputButton code) const override;
		bool isButtonReleased(InputButton code) const override;
		bool isButtonDown(InputButton code) const override;

		bool isButtonPressed(InputButton code, gsl::span<const uint32_t> activeBinds);
		bool isButtonPressedRepeat(InputButton code, gsl::span<const uint32_t> activeBinds);
		bool isButtonReleased(InputButton code, gsl::span<const uint32_t> activeBinds);
		bool isButtonDown(InputButton code, gsl::span<const uint32_t> activeBinds);

		void clearButton(InputButton code) override;
		void clearButtonPress(InputButton code) override;
		void clearButtonRelease(InputButton code) override;

		String getButtonName(int code) const override;

		float getAxis(int n) const override;
		int getAxisRepeat(int n) const override;

		float getAxis(int n, gsl::span<const uint32_t> activeBinds);

		std::pair<float, float> getVibration() const override;
		void setVibration(float low, float high) override;
		void vibrate(spInputVibration vib) override;
		void stopVibrating() override;

		Vector2f getPosition() const override;
		void setPosition(Vector2f pos) override;
		void setPositionLimits(Rect4f limits);
		void setPositionLimits();

		Vector2f getWheelMove() const override;
		Vector2i getWheelMoveDiscrete() const override;

		const InputMotionSensor& getMotionSensor(int n) const override;

		void bindButton(ConvertibleTo<int> n, spInputDevice device, ConvertibleTo<int> deviceButton, bool toggle = false);
		void bindButton(ConvertibleTo<int> n, spInputDevice device, KeyCode deviceButton, std::optional<KeyMods> mods = {}, bool toggle = false);
		void bindButtonChord(ConvertibleTo<int> n, spInputDevice device, ConvertibleTo<int> deviceButton0, ConvertibleTo<int> deviceButton1, bool toggle = false);
		void bindAxis(ConvertibleTo<int> n, spInputDevice device, ConvertibleTo<int> deviceAxis, float scale = 1.0f);
		void bindAxisButton(ConvertibleTo<int> n, spInputDevice device, ConvertibleTo<int> negativeButton, ConvertibleTo<int> positiveButton);
		void bindAxisButton(ConvertibleTo<int> n, spInputDevice device, KeyCode negativeButton, KeyCode positiveButton, std::optional<KeyMods> mods = {});
		void bindVibrationOverride(spInputDevice joy);
		void bindHat(int leftRight, int upDown, spInputDevice hat);
		void bindPosition(spInputDevice device);
		void bindPositionRelative(spInputDevice device, int axisX, int axisY, float speed);
		void bindWheel(spInputDevice device);
		void bindMotionSensor(ConvertibleTo<int> n, spInputDevice device, ConvertibleTo<int> deviceMotionControls);

		void unbindButton(int n);
		void unbindAxis(int n);
		void clearBindings();

		void update(Time t);

		void setRepeat(float first, float hold);

		InputDevice* getLastDevice() const;
		void setLastDeviceToType(InputType type);
		void setLastDeviceFreeze(bool frozen);

		void setJoystickTypeOverride(std::optional<JoystickType> type);
		JoystickType getJoystickType() const override;
		InputType getInputType() const override;

		std::unique_ptr<InputExclusiveButton> makeExclusiveButton(InputButton button, InputPriority priority, const InputLabel& label);
		std::unique_ptr<InputExclusiveAxis> makeExclusiveAxis(int axis, InputPriority priority, const InputLabel& label);

		struct ExclusiveButtonInfo {
			InputButton button;
			InputLabel label;
			InputDevice* physicalDevice;
			int physicalButton;
		};
		Vector<ExclusiveButtonInfo> getExclusiveButtonLabels(InputDevice* preferredDevice);

		std::pair<InputDevice*, int> getPhysicalButton(ConvertibleTo<int> button, InputDevice* preferredDevice) const;
		std::pair<InputDevice*, int> getPhysicalAxis(ConvertibleTo<int> axis, InputDevice* preferredDevice) const;
		std::pair<InputDevice*, int> getPhysicalButton(const InputExclusiveButton& button, InputDevice* device = nullptr) const;
		std::pair<InputDevice*, int> getPhysicalAxis(const InputExclusiveAxis& axis, InputDevice* device = nullptr) const;

		void clearPresses() override;

	private:

		struct Bind {
			enum class Mode : uint8_t {
				Button,
				ButtonToggle,
				Axis,
				AxisEmulation
			};

			spInputDevice device;
			int a = -1;
			int b = -1;
			Mode mode = Mode::Button;
			bool toggleState = false;
			std::optional<KeyMods> mods;
			float scale = 1.0f;

			Bind(spInputDevice d, int a, int b, Mode mode, std::optional<KeyMods> mods = {}, float scale = 1.0f);

			bool isButtonPressed() const;
			bool isButtonPressedRepeat() const;
			bool isButtonReleased() const;
			bool isButtonDown() const;
			bool checkMods() const;

			float getAxis() const;

			void updateToggle();

			std::pair<uint32_t, uint32_t> getPhysicalButtonIds() const;
		};

		struct AxisData {
			Vector<Bind> binds;
			int curRepeatValue = 0;
			InputAxisRepeater repeat;

			AxisData();
			explicit AxisData(Vector<Bind> b);

			float getValue() const;
		};

		struct MotionSensorData {
			spInputDevice device;
			int sensorId;
		};

		struct PositionBindData
		{
			spInputDevice device;
			bool direct = false;
			int axisX = 0;
			int axisY = 0;
			float speed = 0.0f;
			Vector2f lastRead;

			PositionBindData();
			explicit PositionBindData(spInputDevice device);
			explicit PositionBindData(spInputDevice device, int axisX, int axisY, float speed);
		};

		Vector<Vector<Bind>> buttons;
		Vector<AxisData> axes;
		Vector<MotionSensorData> motionSensors;

		Vector<PositionBindData> positions;
		std::optional<Rect4f> positionLimits;
		Vector2f position;

		Vector<spInputDevice> wheels;

		spInputDevice vibrationOverride;
		std::weak_ptr<InputDevice> lastDevice;
		JoystickType lastJoystickType = JoystickType::None;
		std::optional<JoystickType> joystickTypeOverride;
		bool lastDeviceSet = false;
		bool lastDeviceFrozen = false;
		
		float repeatDelayFirst;
		float repeatDelayHold;

		InputType type;

		Vector<InputExclusiveButton*> exclusiveButtons;
		Vector<InputExclusiveAxis*> exclusiveAxes;
		bool exclusiveDirty = true;
		
		void setLastDevice(const std::shared_ptr<InputDevice>& device);
		void updateLastDevice();
		std::set<spInputDevice> getAllDevices() const;

		void addExclusiveButton(InputExclusiveButton& exclusive);
		void removeExclusiveButton(InputExclusiveButton& exclusive);
		void addExclusiveAxis(InputExclusiveAxis& exclusive);
		void removeExclusiveAxis(InputExclusiveAxis& exclusive);
		void refreshExclusives();
		bool checkBinds(gsl::span<const uint32_t> activeBinds, const Bind& bind) const;
	};

	typedef std::shared_ptr<InputVirtual> spInputVirtual;
}
