#pragma once

#include "ui_textinput.h"

namespace Halley {
	class UIFactory;
	class UIStyle;

    class UIDebugConsole : public UIWidget {
    public:
		UIDebugConsole(const String& id, UIFactory& factory);
	};
}
