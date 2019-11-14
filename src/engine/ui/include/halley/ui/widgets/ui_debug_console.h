#pragma once

#include "ui_textinput.h"

namespace Halley {
	class UIFactory;
	class UIStyle;

    class UIDebugConsole : public UIWidget {
    public:
		UIDebugConsole(const String& id, UIFactory& factory);
		void show();
		void hide();
    	
    private:
		void setup();
		void onSubmit();
		void runCommand(const String& command);
		void addLine(const String& line);

		UIFactory& factory;
	};
}
