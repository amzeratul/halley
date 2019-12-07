#pragma once

#include "ui_textinput.h"
#include "halley/concurrency/future.h"

namespace Halley {
	class UIFactory;
	class UIStyle;

	using UIDebugConsoleCallback = std::function<String(std::vector<String>)>;
	using UIDebugConsoleCallbackPair = std::pair<ExecutionQueue*, UIDebugConsoleCallback>;

	class UIDebugConsoleCommands {
	public:
		void addCommand(String command, UIDebugConsoleCallback callback);
		void addAsyncCommand(String command, ExecutionQueue& queue, UIDebugConsoleCallback callback);

		const std::map<String, UIDebugConsoleCallbackPair>& getCommands() const;

	private:
		std::map<String, UIDebugConsoleCallbackPair> commands;
	};

	class UIDebugConsoleController {
	public:
		Future<String> runCommand(String command, std::vector<String> args);
		String runHelp();
		
		void addCommands(UIDebugConsoleCommands& commands);
		void removeCommands(UIDebugConsoleCommands& commands);
		std::vector<StringUTF32> getAutoComplete(const StringUTF32& line) const;

	private:
		std::vector<UIDebugConsoleCommands*> commands;
	};

    class UIDebugConsole : public UIWidget {
    public:
		UIDebugConsole(const String& id, UIFactory& factory, UIDebugConsoleController& controller);
		void show();
		void hide();
		void addLine(const String& line, Colour colour);
    	
    private:
		void setup();
		void onSubmit();
		void runCommand(const String& command);

		UIFactory& factory;
		UIDebugConsoleController& controller;
		Future<String> pendingCommand;
		std::shared_ptr<UITextInput> inputField;
    };
}
