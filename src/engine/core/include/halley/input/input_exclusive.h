#pragma once

#include "input_device.h"
#include "halley/text/string_converter.h"
#include "halley/entity/entity_id.h"

namespace Halley {
    class InputVirtual;

	struct InputLabel {
		String label;
		EntityId target;
        ConfigNode params;

        InputLabel(String label = "", EntityId target = {}, ConfigNode params = {})
	        : label(std::move(label))
			, target(std::move(target))
			, params(std::move(params))
        {}
	};

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
		InputExclusiveButton(InputVirtual& parent, InputPriority priority, InputButton button, InputLabel label);
        ~InputExclusiveButton();

        void update(Time t) override;

        bool isPressed() const;
        bool isPressedRepeat() const;
        bool isReleased() const;
        bool isDown() const;
        bool isActive() const;

        const InputLabel& getLabel() const;

	private:
        InputVirtual* parent = nullptr;
        InputButton button = 0;
        InputPriority priority = InputPriority::Normal;
        Vector<uint32_t> activeBinds;
        InputLabel label;
        
        InputPriority getPriority() const override;
        Vector<uint32_t>& getActiveBinds() override;
	};

	class InputExclusiveAxis : private InputExclusiveBinding {
        friend class InputVirtual;

    public:
		InputExclusiveAxis(InputVirtual& parent, InputPriority priority, int axis, InputLabel label);
        ~InputExclusiveAxis();

        void update(Time t) override;

        float getAxis() const;
        int getAxisRepeat() const;

        const InputLabel& getLabel() const;

	private:
        InputVirtual* parent = nullptr;
        int axis = 0;
        InputPriority priority = InputPriority::Normal;
        Vector<uint32_t> activeBinds;
        InputLabel label;

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
