#pragma once

#include "input_device.h"
#include "halley/text/string_converter.h"

namespace Halley {
    class InputVirtual;

    class InputExclusiveBinding {
    public:
        virtual ~InputExclusiveBinding() = default;
        virtual InputPriority getPriority() const = 0;
        virtual Vector<uint32_t>& getActiveBinds() = 0;
        virtual void update(Time t) = 0;
    };

	class InputExclusiveButton : private InputExclusiveBinding {
        friend class InputVirtual;

    public:
		InputExclusiveButton(InputVirtual& parent, InputPriority priority, InputButton button, String label);
        ~InputExclusiveButton();

        void update(Time t) override;

        bool isPressed() const;
        bool isPressedRepeat() const;
        bool isReleased() const;
        bool isDown() const;
        bool isActive() const;

        const String& getLabel() const;

	private:
        InputVirtual* parent = nullptr;
        InputButton button = 0;
        InputPriority priority = InputPriority::Normal;
        Vector<uint32_t> activeBinds;
        String label;
        
        InputPriority getPriority() const override;
        Vector<uint32_t>& getActiveBinds() override;
	};

	class InputExclusiveAxis : private InputExclusiveBinding {
        friend class InputVirtual;

    public:
		InputExclusiveAxis(InputVirtual& parent, InputPriority priority, int axis, String label);
        ~InputExclusiveAxis();

        void update(Time t) override;

        float getAxis() const;
        int getAxisRepeat() const;

        const String& getLabel() const;

	private:
        InputVirtual* parent = nullptr;
        int axis = 0;
        InputPriority priority = InputPriority::Normal;
        Vector<uint32_t> activeBinds;
        String label;

        InputAxisRepeater repeater;
        int repeatValue;
        
        InputPriority getPriority() const override;
        Vector<uint32_t>& getActiveBinds() override;
    };

    class InputExclusive : public InputDevice {
    public:
        InputExclusive(std::shared_ptr<InputVirtual> input, InputPriority priority, Vector<int> axes, Vector<int> buttons);

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

    	float getAxis(int axis) override;
	    int getAxisRepeat(int axis) override;

    private:
        std::shared_ptr<InputVirtual> input;
        InputPriority priority;
        Vector<int> axes;
        Vector<int> buttons;
        Vector<std::unique_ptr<InputExclusiveButton>> buttonsExclusive;
        Vector<std::unique_ptr<InputExclusiveAxis>> axesExclusive;
        bool enabled = false;
    };
}
