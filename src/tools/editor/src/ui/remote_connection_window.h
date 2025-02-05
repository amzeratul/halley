#pragma once

#include "halley/ui/ui_widget.h"

namespace Halley {
	class ProjectWindow;

    class RemoteConnectionTab : public UIWidget {
    public:
        RemoteConnectionTab(UIFactory& factory, GamePlatform platform, String name);

        void onMakeUI() override;

    private:
        UIFactory& factory;
        GamePlatform& platform;
        String name;
    };

	class RemoteConnectionWindow : public UIWidget, public IUIDebugConsoleController {
    public:
        RemoteConnectionWindow(UIFactory& factory, ProjectWindow& projectWindow, std::shared_ptr<DevConServerConnection> connection, std::shared_ptr<RemoteConnectionTab> tab);

        void onMakeUI() override;

        void update(Time t, bool moved) override;

	protected:
        Future<UIDebugConsoleResponse> runCommand(String command, Vector<String> args) override;
        Future<Vector<StringUTF32>> getAutoComplete(const StringUTF32& line) const override;

    private:
        UIFactory& factory;
        ProjectWindow& projectWindow;
        std::shared_ptr<DevConServerConnection> connection;
        std::shared_ptr<RemoteConnectionTab> tab;

		std::shared_ptr<UIDebugConsole> console;
        std::shared_ptr<UIDebugConsoleController> consoleController;
        std::shared_ptr<UIDebugConsoleCommands> consoleCommands;
        std::shared_ptr<const UIColourScheme> colourScheme;

        void makeCommands();
        void saveRemoteSave(String name, Bytes data) const;
        Future<Bytes> loadRemoteSave(String name) const;
        void addConsoleLine(LoggerLevel level, const String& msg) const;
	};
}
