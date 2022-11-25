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
}
