#pragma once

#include "input_device.h"
#include "halley/text/string_converter.h"

namespace Halley {
    class InputVirtual;

	class InputExclusiveButton {
        friend class InputVirtual;

    public:
		InputExclusiveButton(InputVirtual& parent, InputPriority priority, InputButton button, String label);
        ~InputExclusiveButton();

        bool isPressed() const;
        bool isPressedRepeat() const;
        bool isReleased() const;
        bool isDown() const;

        const String& getLabel() const;

	private:
        InputVirtual* parent = nullptr;
        InputButton button = 0;
        InputPriority priority = InputPriority::Normal;
        Vector<uint32_t> activeBinds;
        String label;
    };

    class InputExclusive : public InputDevice {
    public:
        InputExclusive(std::shared_ptr<InputVirtual> input, Vector<int> axes, Vector<int> buttons);

	    InputType getInputType() const override;

    	void setEnabled(bool enabled);
        bool isEnabled() const override;

        size_t getNumberButtons() override;
	    size_t getNumberAxes() override;
	    String getButtonName(int code) const override;

    	bool isButtonPressed(InputButton code) override;
	    bool isButtonPressedRepeat(InputButton code) override;
	    bool isButtonReleased(InputButton code) override;
	    bool isButtonDown(InputButton code) override;

    	float getAxis(int) override;
	    int getAxisRepeat(int) override;

    private:
        std::shared_ptr<InputVirtual> input;
        Vector<int> axes;
        Vector<int> buttons;
        Vector<std::unique_ptr<InputExclusiveButton>> buttonsExclusive;
        bool enabled = false;
    };
}
